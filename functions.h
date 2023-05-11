#ifndef FUNCTIONS
#define FUNCTIONS

#include "pe_common.h"
#include "dyn_array.h"

extern void initialize_exchange(exchange* pexchange, int argc, char** argv);
extern void handle_products(dyn_array* product_list, FILE* fproducts, int* num_products);
extern trader* initialize_trader(exchange* pexchange, int i, char** argv);
extern void launch_trader(exchange* pexchange, trader* new_trader, int i, char** argv);
extern void market_open(exchange* pexchange);
extern void free_memory(exchange* pexchange);
extern void teardown(exchange* pexchange);
extern bool validate_buysell(char* command);
extern void tell_other_traders(exchange* pexchange, int id, char* message);
extern int read_command(int fd, char* buffer);
extern void print_orderbook(exchange* pexchange);

#endif
