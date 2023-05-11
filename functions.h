#ifndef FUNCTIONS
#define FUNCTIONS

#include "pe_common.h"
#include "dyn_array.h"

void initialize_exchange(exchange* pexchange, int argc, char** argv);
void handle_products(dyn_array* product_list, FILE* fproducts, int* num_products);
trader* initialize_trader(exchange* pexchange, int i, char** argv);
void launch_trader(exchange* pexchange, trader* new_trader, int i, char** argv);
void market_open(exchange* pexchange);
void free_memory(exchange* pexchange);
void teardown(exchange* pexchange);
bool validate_buysell(char* command);
void tell_other_traders(exchange* pexchange, int id, char* message);
int read_command(int fd, char* buffer);

#endif
