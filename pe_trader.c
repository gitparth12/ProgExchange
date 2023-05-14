#include "pe_trader.h"
#include "pe_common.h"
#include <unistd.h>

volatile sig_atomic_t signal_number = 0;
void sigint_handler(int signo) {
    signal_number = signo;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    /*
       sigset_t mask;
       struct sigaction action;
       struct siginfo_t siginfo;

       sigemptyset(&mask);
       sigaddset(&mask, SIGUSR1);
       sigprocmask(SIG_BLOCK, &mask, NULL);
       */

    // register signal handler
    struct sigaction sig = {
        .sa_handler = sigint_handler,
        .sa_flags = SA_SIGINFO
    };

    // Registering signal handler
    if ((sigaction(SIGUSR1, &sig, NULL)) != 0) {
        printf("Error registering signal handler with sigaction");
        return 4;
    }

    // connect to named pipes
    char trader_pipe_path[sizeof(FIFO_TRADER)];
    snprintf(trader_pipe_path, sizeof(FIFO_TRADER), FIFO_TRADER, atoi(argv[1]));
    char exchange_pipe_path[sizeof(FIFO_EXCHANGE)];
    snprintf(exchange_pipe_path, sizeof(FIFO_EXCHANGE), FIFO_EXCHANGE, atoi(argv[1]));

    int trader_pipe;
    int exchange_pipe;
    // Opening both pipes
    if ((exchange_pipe = open(exchange_pipe_path, O_RDONLY)) == -1) {
        printf("Error opening exchange_pipe\n");
        return 2;
    }
    if ((trader_pipe = open(trader_pipe_path, O_WRONLY)) == -1) {  // open trader pipe for writing
        printf("Error opening trader_pipe\n");
        return 2;
    }

    char message[BUF_SIZE];

    // Check for the MARKET OPEN command, and start listening for orders after
    pause();
    if (signal_number == SIGUSR1) {
        signal_number = 0;
        read(exchange_pipe, message, BUF_SIZE);  // Read from exchange pipe
    }

    int order_id = 0;
    // event loop:
    while (1) {
        memset(message, 0, BUF_SIZE);
        pause();
        if (signal_number == SIGUSR1) {
            // Read from exchange pipe
            read(exchange_pipe, message, BUF_SIZE);
            signal_number = 0;
        }

        if (strncmp(message, "MARKET SELL ", strlen("MARKET SELL ")) == 0) {

            char product[PROD_SIZE] = {0};
            long qty = 0;
            long price = 0;

            sscanf(message, "%*s %*s %s %ld %ld;", product, &qty, &price);

            if (qty >= 1000) {
                // printf("DEBUG: quantity over 1000\n");
                close(trader_pipe);
                close(exchange_pipe);
                return 1;
            }

            // send order
            memset(message, 0, BUF_SIZE);
            snprintf(message, BUF_SIZE, "BUY %d %s %ld %ld;", order_id++, product, qty, price);
            write(trader_pipe, message, strlen(message));
            kill(getppid(), SIGUSR1);

            // wait for exchange confirmation (ACCEPTED message)
            memset(message, 0, BUF_SIZE);
            pause();
            if (signal_number == SIGUSR1) {
                read(exchange_pipe, message, BUF_SIZE);
                signal_number = 0;
            }
        }
    }
}
