#include "functions.h"

int read_command(int fd, char* buffer) {
    char temp = 0;
    int i = 0;
    while (temp != ';') {
        if (read(fd, &temp, 1) == -1) {
            printf("Error while reading command.\nRead so far: %s\n", buffer);
            return -1;
        }
        if (temp != ';')
            buffer[i++] = temp;
    }
    return 1;
}

void tell_other_traders(exchange* pexchange, int id, char* message) {
    for (int i = 0; i < pexchange->traders->size; i++) {
        trader* current = (trader*) dyn_array_get(pexchange->traders, i);
        if (current->id == id)
            continue;
        if (write(current->exchange_pipe, message, strlen(message)) == -1) {
            printf("trader_id: %d\nError writing message: %s\n", current->id, message);
            perror("Something went wrong: ");
            exit(4);
        }
        kill(current->pid, SIGUSR1);
    }
}

bool validate_buysell(char* command) {
    char order_id_string[INT_STRING_SIZE + 1];
    char product_name[PROD_SIZE + 1];
    char qty_string[INT_STRING_SIZE + 1];
    char price_string[INT_STRING_SIZE + 1];
    if (sscanf(command, "%*s %s %s %s %s", order_id_string, product_name, qty_string, price_string) != 4) {
        printf("sscanf error\n");
        return false;
    }
    // error checking with integer values in command
    int order_id;
    int qty;
    int price;
    if ((order_id = atoi(order_id_string)) == 0 && (strncmp(order_id_string, "0", strlen(order_id_string))) != 0) {
        printf("order_id error\n");
        return false;
    }
    if ((qty = atoi(qty_string)) == 0) {
        printf("qty error\n");
        return false;
    }
    if ((price = atoi(price_string)) == 0) {
        printf("price error\n");
        return false;
    }
    return true;
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

    char temp[PRODUCT_NAME_SIZE + 1];
    for (int i = 0; i < *num_products; i++) {
        product* new_product = (product*) malloc(sizeof(product));
        fgets(temp, PRODUCT_NAME_SIZE, fproducts);
        temp[strcspn(temp, "\n")] = 0;
        asprintf(&new_product->name, "%s", temp);
        new_product->buy_prices = dyn_array_init();
        new_product->sell_prices = dyn_array_init();
        dyn_array_add(product_list, (void*) new_product);
        // testing purposes
        product* test = (product*) dyn_array_get(product_list, i);
        printf(" %s", test->name);
    }
    printf("\n");
    fclose(fproducts);
}

trader* initialize_trader(exchange* pexchange, int i, char** argv) {
    char* path;
    // Make a new trader
    trader* new_trader = (trader*) malloc(sizeof(trader));
    new_trader->id = i-2;

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
        perror("Error forking");
    }
    if (pid == 0) { // child process
                    // exec the child to trader binary
        char* id;
        asprintf(&id, "%d", i-2);
        execl(new_trader->binary, strrchr(new_trader->binary, '/'), id, (char*) NULL);            
        printf("\nTrader id: %d\n", i);
        perror("Error during exec");
        free(id);
        // fclose(fproducts);
        free_memory(pexchange);
        exit(2);
    }
    else {
        // store pid in trader struct
        new_trader->pid = pid;
    }

    // CONNECT TO PIPES
    if ((new_trader->exchange_pipe = open(new_trader->exchange_pipe_path, O_WRONLY)) == -1) {
        printf("\nTrader id: %d\n", i);
        perror("Error opening exchange_pipe");
    }
    else {
        if ((new_trader->fexchange_pipe = fdopen(new_trader->exchange_pipe, "w")) == NULL) {
            printf("\nTrader id: %d\n", i);
            perror("Error making file pointer for exchange pipe.");
        }
        printf("%s Connected to %s\n", LOG_PREFIX, new_trader->exchange_pipe_path);
    }
    if ((new_trader->trader_pipe = open(new_trader->trader_pipe_path, O_RDONLY)) == -1) {
        printf("\nTrader id: %d\n", i);
        perror("Error opening trader_pipe");
    }
    else {
        if ((new_trader->ftrader_pipe = fdopen(new_trader->trader_pipe, "r")) == NULL) {
            printf("\nTrader id: %d\n", i);
            perror("Error making file pointer for trader pipe.");
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
            perror("Error writing MARKET OPEN;");
            exit(3);
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
    dyn_array_free_values(pexchange->sigusr_pids);
    dyn_array_free(pexchange->sigusr_pids);
    dyn_array_free_traders(pexchange->traders);
    dyn_array_free(pexchange->traders);
    dyn_array_free_products(pexchange->product_list);
    dyn_array_free(pexchange->product_list);
}

void teardown(exchange* pexchange) {
    printf("%s Trading completed\n", LOG_PREFIX);
    printf("%s Exchange fees collected: $%d\n", LOG_PREFIX, pexchange->fee);
    free_memory(pexchange);
}
/*
   void* my_malloc(size_t size) {
   return malloc(size);
   }
   */
