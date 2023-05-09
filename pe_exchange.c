/**
 * comp2017 - assignment 3
 * Parth Bhargava
 * pbha6036
 */

#include "pe_exchange.h"


volatile sig_atomic_t signo = 0;
volatile siginfo_t siginfo;
bool sigusr = false;
void handler1(int signal_num, siginfo_t* info, void* ucontext) {
    sigusr = true;
    signo = signal_num;
    siginfo = *info;
}

bool sigpipe = false;
void handler2(int signal_num, siginfo_t* info, void* ucontext) {
    sigpipe = true;
    signo = signal_num;
    siginfo = *info;
}

bool sigchld = false;
void handler3(int signal_num, siginfo_t* info, void* ucontext) {
    sigchld = true;
    signo = signal_num;
    siginfo = *info;
}


/*
void sighandler(int random, siginfo_t *info) {
    info->si_pid;
}

sigaction(SIGUSR1, &sig, NULL);
sigaction(SIGCHLD, &sig, NULL);
sigaction(SIGPIPE, )
*/

int main(int argc, char** argv) {
    // ERROR CHECKING
    // number of command line arguments
    if (argc < 3) {
        printf("%s Not enough command line arguments.\n", LOG_PREFIX);
        exit(1);
    }

    // register signal handler
    struct sigaction sig = {
        .sa_sigaction = &handler1,
        .sa_flags = SA_SIGINFO
    };

    // Registering signal handler
    if ((sigaction(SIGUSR1, &sig, NULL)) != 0) {
        printf("Error registering signal handler with sigaction");
        return 4;
    }
    sig.sa_sigaction = &handler2;
    if ((sigaction(SIGPIPE, &sig, NULL)) != 0) {
        printf("Error registering signal handler with sigaction");
        return 4;
    }
    sig.sa_sigaction = &handler3;
    if ((sigaction(SIGCHLD, &sig, NULL)) != 0) {
        printf("Error registering signal handler with sigaction");
        return 4;
    }

    // Making exchange struct
    exchange exchange_data = {
        .product_list = dyn_array_init(),
        .traders = dyn_array_init(),
        .num_products = 0,
    };
    exchange* pexchange = &exchange_data;

    initialize_exchange(pexchange, argc, argv);

    // Event loop
    while (1) {
        // check for SIGPIPE
        if (sigpipe) {
            sigpipe = false;
            for (int i = 0; i < pexchange->traders->size; i++) {
                trader* current = pexchange->traders->array[i];
                if (current->pid == siginfo.si_pid) {
                    printf("%s Trader %d disconnected\n", LOG_PREFIX, i);
                    close(current->trader_pipe);
                    close(current->exchange_pipe);
                    unlink(current->trader_pipe_path);
                    unlink(current->exchange_pipe_path);
                    dyn_array_delete_trader(pexchange->traders, i);
                }
            }
        }
    }

    // Free all allocated memory from dynamic arrays
    free_memory(pexchange);
    return 0;
}

void initialize_exchange(exchange* pexchange, int argc, char** argv) {
    // Start message
    printf("%s Starting\n", LOG_PREFIX);

    // Read product file
    FILE* fproducts;
    if ((fproducts = fopen(argv[1], "r")) == NULL) {
        printf("Couldn't open the product file, something went wrong :(\n");
        exit(2);
    }
    handle_products(pexchange->product_list, fproducts, &pexchange->num_products);

    for (int i = 2; i < argc; i++) {
        // Initialise trader and make fifos for it
        trader* new_trader = initialize_trader(pexchange, i, argv);
        // Launch the trader binary
        launch_trader(pexchange, new_trader, i, argv);
    }

    // Market open
    market_open(pexchange);
}

void handle_products(dyn_array* product_list, FILE* fproducts, int* num_products) {
    fscanf(fproducts, "%d\n", num_products);
    printf("%s Trading %d products:", LOG_PREFIX, *num_products);

    char prod_name[PRODUCT_NAME_SIZE + 1];
    for (int i = 0; i < *num_products; i++) {
        fgets(prod_name, PRODUCT_NAME_SIZE, fproducts);
        prod_name[strcspn(prod_name, "\n")] = 0;
        char* product;
        asprintf(&product, "%s", prod_name);
        dyn_array_add(product_list, (void*) product);
        printf(" %s", (char*)product_list->array[product_list->size-1]);
    }
    printf("\n");
    fclose(fproducts);
}

trader* initialize_trader(exchange* pexchange, int i, char** argv) {
    char* path;
    // Make a new trader
    trader* new_trader = (trader*) malloc(sizeof(trader));

    asprintf(&new_trader->binary, "%s", argv[i]);
    // exchange fifo
    asprintf(&path, FIFO_EXCHANGE, i-2);
    asprintf(&new_trader->exchange_pipe_path, "%s", path);
    if (mkfifo(path, 0666) == -1) {
        printf("\nTrader id: %d\n", i-2);
        perror("Error making exchange fifo");
    }
    printf("%s Created FIFO %s\n", LOG_PREFIX, path);
    free(path);
    // trader fifo
    asprintf(&path, FIFO_TRADER, i-2);
    asprintf(&new_trader->trader_pipe_path, "%s", path);
    if (mkfifo(path, 0666) == -1) {
        printf("\nTrader id: %d\n", i-2);
        perror("Error making trader fifo");
    }
    printf("%s Created FIFO %s\n", LOG_PREFIX, path);
    free(path);

    // Adding trader to list
    dyn_array_add(pexchange->traders, (void*) new_trader);
    return new_trader;
}

void launch_trader(exchange* pexchange, trader* new_trader, int i, char** argv) {
    pid_t pid;
    printf("%s Starting trader %d (%s)\n", LOG_PREFIX, i-2, new_trader->binary);
    if ((pid = fork()) < 0) {
        printf("\nTrader id: %d\n", i);
        perror("Error forking\n");
    }
    if (pid == 0) { // child process
                    // exec the child to trader binary
        char* id;
        asprintf(&id, "%d", i-2);
        execl(new_trader->binary, strrchr(new_trader->binary, '/'), id, (char*) NULL);            
        printf("\nTrader id: %d\n", i);
        perror("Error during exec\n");
        free(id);
        // fclose(fproducts);
        dyn_array_free_values(pexchange->product_list);
        dyn_array_free(pexchange->product_list);
        dyn_array_free_traders(pexchange->traders);
        dyn_array_free(pexchange->traders);
        exit(2);
    }
    else {
        // store pid in trader struct
        new_trader->pid = pid;
    }

    // CONNECT TO PIPES
    if ((new_trader->exchange_pipe = open(new_trader->exchange_pipe_path, O_WRONLY)) == -1) {
        printf("\nTrader id: %d\n", i);
        perror("Error opening exchange_pipe\n");
    }
    else {
        if ((new_trader->fexchange_pipe = fdopen(new_trader->exchange_pipe, "w")) == NULL) {
            printf("\nTrader id: %d\n", i);
            perror("Error making file pointer for exchange pipe.\n");
        }
        printf("%s Connected to %s\n", LOG_PREFIX, new_trader->exchange_pipe_path);
    }
    if ((new_trader->trader_pipe = open(new_trader->trader_pipe_path, O_RDONLY)) == -1) {
        printf("\nTrader id: %d\n", i);
        perror("Error opening trader_pipe\n");
    }
    else {
        if ((new_trader->ftrader_pipe = fdopen(new_trader->trader_pipe, "r")) == NULL) {
            printf("\nTrader id: %d\n", i);
            perror("Error making file pointer for trader pipe.\n");
        }
        printf("%s Connected to %s\n", LOG_PREFIX, new_trader->trader_pipe_path);
    }
}

void market_open(exchange* pexchange) {
    for (int i = 0; i < pexchange->traders->size; i++) {
        trader* current = (trader*) pexchange->traders->array[i];
        char* message = "MARKET OPEN;";
        if (write(current->exchange_pipe, message, strlen(message)) == -1) {
            printf("\nTrader id: %d\n", i);
            perror("Error writing MARKET OPEN;\n");
        }
        kill(current->pid, SIGUSR1);
    }
}

void free_memory(exchange* pexchange) {
    for (int i = 0; i < pexchange->traders->size; i++) {
        trader* current = (trader*) pexchange->traders->array[i];
        close(current->trader_pipe);
        unlink(current->trader_pipe_path);
        close(current->exchange_pipe);
        unlink(current->exchange_pipe_path);
    }
    dyn_array_free_values(pexchange->product_list);
    dyn_array_free(pexchange->product_list);
    dyn_array_free_traders(pexchange->traders);
    dyn_array_free(pexchange->traders);
}

/*
   void* my_malloc(size_t size) {
   return malloc(size);
   }
   */
