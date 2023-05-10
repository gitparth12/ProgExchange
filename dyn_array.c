/**
 * comp2017 - assignment 3
 * Parth Bhargava
 * pbha6036
 * SID: 510401900
 * This file contains acknowledgements of code
 * USYD CODE CITATION ACKNOWLEDGEMENT
 */

#include "dyn_array.h"

/*
 * USYD CODE CITATION ACKNOWLEDGEMENT
 * I declare that the majority of the lines of code below has been taken from
 * the Angus O'Grady's Tutorial 4 question 5 and is not my own work.
 */

void dyn_resize(dyn_array *dyn) {
    // Double the size of the internal array.
    dyn->capacity *= 2;
    dyn->array = realloc(dyn->array, dyn->capacity * sizeof(void*));

    if (dyn->array == NULL) {
        perror("dyn_resize()");
        exit(1);
    }

    return;
}

dyn_array* dyn_array_init() {
    // Initialise a new dynamic array.
    dyn_array *dyn = calloc(1, sizeof(dyn_array));
    dyn->capacity = INITIAL_CAPACITY;
    dyn->array = malloc(dyn->capacity * sizeof(void*));

    if (dyn->array == NULL) {
        perror("dyn_array_init()");
        exit(1);
    }

    return dyn;
}

void dyn_array_add(dyn_array* dyn, void* value) {
    // Add a value to the end of the dynamic array.
    if (dyn->size + 1 > dyn->capacity) {
        // We need to resize.
        dyn_resize(dyn);
    }

    dyn->array[dyn->size] = value;
    dyn->size++;
    return;
}

void dyn_array_delete(dyn_array *dyn, int index) {
    // We shift everything to the right of the index left by one.
    int i = index;
    while (i < dyn->size - 1) {
        dyn->array[i] = dyn->array[i + 1];
        i++;
    }

    dyn->size--;
    return;
}

void* dyn_array_get(dyn_array *dyn, int index) {
    // Check the bound on the index.
    if (!(0 <= index && index < dyn->size)) {
        fprintf(stderr, "dyn_array_get(): Tried to access index out of bounds.");
        exit(1);
    }

    return dyn->array[index];
}

int dyn_array_get_index(dyn_array *dyn, void* value) {
    for (int i = 0; i < dyn->size; i++) {
        if (dyn_array_get(dyn, i) == value){
            return i;
        }
    }
    return -1;
}

void dyn_array_free(dyn_array *dyn) {
    free(dyn->array);
    free(dyn);
    return;
}

void dyn_array_free_values(dyn_array* dyn) {
    for (int i = 0; i < dyn->size; i++) {
        free(dyn->array[i]);
    }
    return;
}

void dyn_array_free_traders(dyn_array* dyn) {
    for (int i = 0; i < dyn->size; i++) {
        // free trader attributes
        free(((trader*)(dyn->array[i]))->binary);
        free(((trader*)(dyn->array[i]))->exchange_pipe_path);
        free(((trader*)(dyn->array[i]))->trader_pipe_path);
        // free traders themselves
        free(dyn->array[i]);
    }
    return;
}

void dyn_array_delete_trader(dyn_array* dyn, int index) {
    int i = index;
    // free trader memory and clean up pipes
    close(((trader*)(dyn->array[i]))->trader_pipe);
    close(((trader*)(dyn->array[i]))->exchange_pipe);
    unlink(((trader*)(dyn->array[i]))->trader_pipe_path);
    unlink(((trader*)(dyn->array[i]))->exchange_pipe_path);
    free(((trader*)(dyn->array[i]))->binary);
    free(((trader*)(dyn->array[i]))->exchange_pipe_path);
    free(((trader*)(dyn->array[i]))->trader_pipe_path);
    // free traders themselves
    free(dyn->array[i]);
    
    // We shift everything to the right of the index left by one.
    while (i < dyn->size - 1) {
        dyn->array[i] = dyn->array[i + 1];
        i++;
    }

    dyn->size--;
    return;
}

void dyn_array_free_products(dyn_array* dyn) {
    // free products_list and everything inside
    for (int i = 0; i < dyn->size; i++) {
        product* prod = (product*) dyn->array[i];
        // free buy_prices
        for (int j = 0; j < prod->buy_prices->size; j++) {
            price_entry* price = (price_entry*) prod->buy_prices->array[j];                        
            for (int k = 0; k < price->orders->size; k++) {
                free(price->orders->array[k]);
            }
            free(price->orders);
            free(price);
        }
        free(prod->buy_prices);
        // free sell_prices
        for (int j = 0; j < prod->sell_prices->size; j++) {
            price_entry* price = (price_entry*) prod->sell_prices->array[j];                        
            for (int k = 0; k < price->orders->size; k++) {
                free(price->orders->array[k]);
            }
            free(price->orders);
            free(price);
        }
        free(prod->sell_prices);
        // free product
        free(prod); 
    }
    free(dyn);
}

/* End of copied code */
