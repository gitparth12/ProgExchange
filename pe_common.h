#ifndef PE_COMMON_H
#define PE_COMMON_H

// #define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <math.h>

#define FIFO_EXCHANGE "/tmp/pe_exchange_%d"
#define FIFO_TRADER "/tmp/pe_trader_%d"
#define FEE_PERCENTAGE 1
#define BUF_SIZE (64)
#define PROD_SIZE (16)
#define INT_STRING_SIZE (6)
#define LOG_PREFIX "[PEX]"
#define PRODUCT_NAME_SIZE (16)
#define COMMAND_SIZE (44)

typedef struct {
  int size;
  int capacity;
  void** array;
} dyn_array;

typedef struct{
    dyn_array* product_list;
    dyn_array* traders;
    int num_products;
    int fee;
    dyn_array* sigusr_pids;
} exchange;

typedef struct{
    long qty;
    long net_value;
} position;

typedef struct{
    char* binary;
    int exchange_pipe;
    char* exchange_pipe_path;
    int trader_pipe;
    char* trader_pipe_path;
    pid_t pid;
    int id;
    // array of position structs
    position* positions; // indices map to product_ls
    bool connected;
    int last_order;
    dyn_array* orders;
} trader;

typedef struct{
    long value;
    dyn_array* orders; // order structs
} price_entry;

typedef struct{
    char* name;
    dyn_array* buy_prices; // price structs
    dyn_array* sell_prices; // price structs
} product;

typedef enum {
    BUY,
    SELL,
    AMEND,
    CANCEL,
    INVALID,
} command;

typedef struct{
    int order_id;
    long qty;
    trader* source;
    product* prod;
    price_entry* price;
    command order_type;
    int index;
} order;


#endif
