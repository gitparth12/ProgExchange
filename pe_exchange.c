/**
 * comp2017 - assignment 3
 * Parth Bhargava
 * pbha6036
 */

#include "pe_exchange.h"
#include "dyn_array.h"
#include "pe_common.h"


volatile sig_atomic_t signo = 0;
volatile siginfo_t siginfo;
dyn_array* sigusr_pids;
volatile bool sigusr = false;
void handler1(int signal_num, siginfo_t* info, void* ucontext) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    // Do some work that may take a while
    sigusr = true;
    // signo = signal_num;
    // siginfo = *info;
    pid_t* pid = (pid_t*) malloc(sizeof(pid_t));
    *pid = info->si_pid;
    dyn_array_add(sigusr_pids, (void*) pid);

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

volatile bool sigpipe = false;
void handler2(int signal_num, siginfo_t* info, void* ucontext) {
    sigpipe = true;
    signo = signal_num;
    siginfo = *info;
}

volatile bool sigchld = false;
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
    // number of buffer line arguments
    if (argc < 3) {
        printf("%s Not enough buffer line arguments.\n", LOG_PREFIX);
        exit(1);
    }

    // initialize sigusr1 dynarray
    sigusr_pids = dyn_array_init();
    // register signal handler
    struct sigaction sig = {
        .sa_sigaction = &handler1,
        .sa_flags = SA_SIGINFO
    };
    sigemptyset(&sig.sa_mask);
    sigaddset(&sig.sa_mask, SIGUSR1);

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
        .fee = 0,
        .sigusr_pids = sigusr_pids,
    };
    exchange* pexchange = &exchange_data;

    initialize_exchange(pexchange, argc, argv);

    // Event loop
    while (1) {
        int connected_traders = 0;
        for (int i = 0; i < pexchange->traders->size; i++) {
            trader* current = (trader*) dyn_array_get(pexchange->traders, i);
            if (current->connected)
                connected_traders++;
        }
        if (connected_traders == 0) {
            teardown(pexchange);
            break;
        }
        /*
        if (pexchange->traders->size == 0) {
            teardown(pexchange);
            break;
        }
        */
        // check for SIGPIPE or SIGCHLD
        if (sigpipe || sigchld) {
            sigpipe = false;
            sigchld = false;
            for (int i = 0; i < pexchange->traders->size; i++) {
                trader* current = (trader*) dyn_array_get(pexchange->traders, i);
                if (current->pid == siginfo.si_pid) {
                    printf("%s Trader %d disconnected\n", LOG_PREFIX, current->id);
                    // dyn_array_delete_trader(pexchange->traders, i);
                    current->connected = false;
                    close(current->trader_pipe);
                    close(current->exchange_pipe);
                    unlink(current->trader_pipe_path);
                    unlink(current->exchange_pipe_path);
                }
            }
        }
        // Main SIGUSR1 stuff
        while(pexchange->sigusr_pids->size != 0) {
            // get pid of sigusr1 and the corresponding trader
            pid_t pid = *((pid_t*) dyn_array_get(pexchange->sigusr_pids, 0));
            // printf("pid: %ld\n", (long) pid);
            trader* source;
            if ((source = dyn_array_get_trader(pexchange->traders, pid)) == NULL) {
                printf("Source of sigusr1 (trader) doesn't exist in list.\n");
                free(dyn_array_get(pexchange->sigusr_pids, 0));
                dyn_array_delete(pexchange->sigusr_pids, 0);
                continue;
            }
            if (source->connected == false) {
                free(dyn_array_get(pexchange->sigusr_pids, 0));
                dyn_array_delete(pexchange->sigusr_pids, 0);
                continue;
            }
            // scan input from that trader's pipe
            char buffer[BUF_SIZE] = {0};
            // read_command(source->trader_pipe, buffer);
            
            if (read_command(source->trader_pipe, buffer) == -1) {
                printf("Couldn't read from trader pipe.\n");
                perror("read error: ");
                free(dyn_array_get(pexchange->sigusr_pids, 0));
                dyn_array_delete(pexchange->sigusr_pids, 0);
                continue;
            }
            

            // check if message fits in max buffer size
            if (buffer[BUF_SIZE-1] != '\0') {
                printf("\nMessage from trader too long\n");
                printf("%s\n\n", buffer);
                free(dyn_array_get(pexchange->sigusr_pids, 0));
                dyn_array_delete(pexchange->sigusr_pids, 0);
                continue;
            }
            // PROCESS MESSAGE
            command command_type;
            if ((strncmp(buffer, "BUY", strlen("BUY"))) == 0) {
                command_type = BUY;
            }
            else if ((strncmp(buffer, "SELL", strlen("SELL"))) == 0) {
                command_type = SELL;
            }
            else if ((strncmp(buffer, "AMEND", strlen("AMEND"))) == 0) {
                command_type = AMEND;
            }
            else if ((strncmp(buffer, "CANCEL", strlen("CANCEL"))) == 0) {
                command_type = CANCEL;
            }
            else {
                command_type = INVALID;
            }

            // printf("FROM TRADER: %s\n", buffer);
            printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, source->id, buffer);
            switch (command_type) {
                case BUY:
                case SELL:;
                    // store everything in variables
                    int order_id;
                    char product_name[PROD_SIZE + 1] = {0};
                    int qty;
                    int price;
                    if (sscanf(buffer, "%*s %d %16s %d %d", &order_id, product_name, &qty, &price) != 4) {
                        // printf("Malformed buffer: %s\n", buffer);
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // Error checking (for INVALID cases)
                    // --> invalid price, qty or order_id

                    if (price <= 0 || price > 999999 || \
                            qty <= 0 || qty > 999999 || \
                            order_id != source->last_order + 1) {
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // store to orderbook
                    order* new_order = store_product(pexchange, source, command_type, order_id, product_name, qty, price);
                    // check INVALID
                    if (new_order == NULL) {
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // update order_id
                    source->last_order = order_id;
                    // write to pipe
                    char* message;
                    asprintf(&message, "ACCEPTED %d;", order_id);
                    write(source->exchange_pipe, message, strlen(message));
                    free(message);
                    // send signal
                    kill(source->pid, SIGUSR1);
                    // write to all other traders and send signals
                    if (command_type == BUY)
                        asprintf(&message, "MARKET BUY %s %d %d;", product_name, qty, price);
                    else if (command_type == SELL)
                        asprintf(&message, "MARKET SELL %s %d %d;", product_name, qty, price);
                    tell_other_traders(pexchange, source->id, message);                
                    free(message);
                    // Try to match order
                    match_order(pexchange, command_type, product_name, price, new_order, source);
                    // print orderbook
                    print_report(pexchange);
                    break;
                case AMEND:;
                    // store everything in variables
                    /* int order_id; */
                    /* int qty; */
                    /* int price; */
                    if (sscanf(buffer, "%*s %d %d %d", &order_id, &qty, &price) != 3) {
                        // printf("Malformed buffer: %s\n", buffer);
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // Error checking (for INVALID cases)
                    // --> invalid price, qty or order_id
                    // 1. price, order_id, qty
                    if (price <= 0 || price > 999999 || \
                            qty <= 0 || qty > 999999) {
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // Find the order to amend
                    order* found = NULL;
                    for (int i = 0; i < source->orders->size; i++) {
                        order* current = (order*) dyn_array_get(source->orders, i);
                        if (current->order_id == order_id) {
                            found = current;
                            break;
                        }
                    }
                    // check if order is found
                    if (found == NULL) {
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // Actually amend the found order
                    amend_order(pexchange, found, qty, price);
                    asprintf(&message, "AMMENDED %d;", order_id);
                    write(source->exchange_pipe, message, strlen(message));
                    free(message);
                    // send signal
                    kill(source->pid, SIGUSR1);
                    // tell all other traders
                    message = NULL;
                    if (found->order_type == BUY)
                        asprintf(&message, "MARKET BUY %s %d %d;", found->prod->name, qty, price);
                    else if (found->order_type == SELL)
                        asprintf(&message, "MARKET SELL %s %d %d;", found->prod->name, qty, price);
                    tell_other_traders(pexchange, source->id, message);
                    free(message);
                    print_report(pexchange);
                    break;

                case CANCEL:
                    if (sscanf(buffer, "%*s %d", &order_id) != 1) {
                        // printf("Malformed buffer: %s\n", buffer);
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // Find the order to cancel
                    found = NULL;
                    for (int i = 0; i < source->orders->size; i++) {
                        order* current = (order*) dyn_array_get(source->orders, i);
                        if (current->order_id == order_id) {
                            found = current;
                            break;
                        }
                    }
                    // check if order is found
                    if (found == NULL) {
                        char* message;
                        asprintf(&message, "INVALID;");
                        write(source->exchange_pipe, message, strlen(message));
                        free(message);
                        // send signal
                        kill(source->pid, SIGUSR1);
                        // remove current pid
                        free(dyn_array_get(pexchange->sigusr_pids, 0));
                        dyn_array_delete(pexchange->sigusr_pids, 0);
                        continue;
                    }
                    // Actually cancel the order
                    product* prod = found->prod;
                    command type = found->order_type;
                    cancel_order(pexchange, found);
                    message = NULL;
                    asprintf(&message, "CANCELLED %d;", order_id);
                    write(source->exchange_pipe, message, strlen(message));
                    free(message);
                    // send signal
                    kill(source->pid, SIGUSR1);
                    // tell all other traders
                    message = NULL;
                    if (type == BUY)
                        asprintf(&message, "MARKET BUY %s %d %d;", prod->name, 0, 0);
                    else if (type == SELL)
                        asprintf(&message, "MARKET SELL %s %d %d;", prod->name, 0, 0);
                    tell_other_traders(pexchange, source->id, message);
                    free(message);
                    print_report(pexchange);
                    break;

                case INVALID:;
                    char* inv_message;
                    asprintf(&inv_message, "INVALID;");
                    write(source->exchange_pipe, inv_message, strlen(inv_message));
                    free(inv_message);
                    // send signal
                    kill(source->pid, SIGUSR1);
                    break;
            }
            // remove current sigusr1 from backlog
            free(dyn_array_get(pexchange->sigusr_pids, 0));
            dyn_array_delete(pexchange->sigusr_pids, 0);
        }
    }

    /*
    for (int i = 0; i < sigusr_pids->size; i++) {
        printf("%ld\n", (long) *((pid_t*)sigusr_pids->array[i]));
    }
    */
    return 0;
}


