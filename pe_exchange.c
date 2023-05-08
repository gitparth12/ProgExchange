/**
 * comp2017 - assignment 3
 * Parth Bhargava
 * pbha6036
 * SID: 510401900
 * This file contains acknowledgements of code
 * USYD CODE CITATION ACKNOWLEDGEMENT
 */

#include "pe_exchange.h"
#include "dyn_array.h"
#include "pe_common.h"

void handle_products(dyn_array* product_list, FILE* fproducts, int* num_products);

int main(int argc, char** argv) {
    // ERROR CHECKING
    // number of command line arguments
    if (argc < 3) {
        printf("%s Not enough command line arguments.\n", LOG_PREFIX);
        exit(1);
    }

    // Making exchange struct
    exchange exchange_data = {
        .product_list = dyn_array_init(),
        .traders = dyn_array_init(),
        .num_products = 0,
    };
    exchange* pexchange = &exchange_data;

    // Start message
    printf("%s Starting\n", LOG_PREFIX);

    // Read product file
    FILE* fproducts;
    if ((fproducts = fopen(argv[1], "r")) == NULL) {
        printf("Couldn't open the product file, something went wrong :(\n");
        exit(2);
    }
    handle_products(pexchange->product_list, fproducts, &pexchange->num_products);

    // Make fifos and add traders to dyn_array
    char* path;
    for (int i = 2; i < argc; i++) {
        // Make a new trader
        trader* new_trader = (trader*) malloc(sizeof(trader));
        asprintf(&new_trader->binary, "%s", argv[i]);
        // exchange fifo
        asprintf(&path, FIFO_EXCHANGE, i-2);
        asprintf(&new_trader->exchange_pipe, "%s", path);
        if (mkfifo(path, 0666) == -1) {
            printf("\nTrader id: %d\n", i);
            perror("Error making exchange fifo");
        }
        free(path);
        // trader fifo
        asprintf(&path, FIFO_TRADER, i-2);
        asprintf(&new_trader->trader_pipe, "%s", path);
        if (mkfifo(path, 0666) == -1) {
            printf("\nTrader id: %d\n", i);
            perror("Error making trader fifo");
        }
        free(path);
        // Adding trader to list
        // printf("%s\n", new_trader->exchange_pipe);
        dyn_array_add(pexchange->traders, (void*) new_trader);
    }
    
    // test printing traders
    /*
    for (int i = 0; i < pexchange->traders->size; i++) {
        printf("%s", ((trader*)(pexchange->traders->array[i]))->binary);
    }
    */


    // FREE all dynamic arrays and close files and stuff
    fclose(fproducts);
    dyn_array_free_values(pexchange->product_list);
    dyn_array_free(pexchange->product_list);
    dyn_array_free_traders(pexchange->traders);
    dyn_array_free(pexchange->traders);

    return 0;
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
}

/*
   void* my_malloc(size_t size) {
   return malloc(size);
   }
   */
