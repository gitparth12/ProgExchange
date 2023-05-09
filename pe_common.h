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

#define FIFO_EXCHANGE "/tmp/pe_exchange_%d"
#define FIFO_TRADER "/tmp/pe_trader_%d"
#define FEE_PERCENTAGE 1
#define BUF_SIZE (64)
#define PROD_SIZE (16)

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
} exchange;

typedef struct{
    char* binary;
    int exchange_pipe;
    FILE* fexchange_pipe;
    char* exchange_pipe_path;
    int trader_pipe;
    FILE* ftrader_pipe;
    char* trader_pipe_path;
    pid_t pid;
    int id;
} trader;

typedef enum {
    ACCEPTED,
    AMENDED,
    CANCELLED,
    INVALID,
} command;

#endif
