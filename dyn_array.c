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
 * Angus O'Grady's Tutorial 4 question 5 and is not my own work.
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

void dyn_array_add_price(dyn_array* dyn, void* value) {
    // Add a value to the end of the dynamic array.
    if (dyn->size + 1 > dyn->capacity) {
        // We need to resize.
        dyn_resize(dyn);
    }

    dyn->array[dyn->size] = value;
    dyn->size++;

    // sort the whole array
    for (int i = 0; i < dyn->size; i++) {
        price_entry* current_price = (price_entry*) dyn->array[i];
        int j = i-1;
        while (j >= 0 && ((price_entry*) dyn->array[j])->value > current_price->value) {
            // swap array[j+1] and array[j]
            void* temp = dyn->array[j];
            dyn->array[j] = dyn->array[j+1];
            dyn->array[j+1] = temp;
            j--;
        }
    }
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

trader* dyn_array_get_trader(dyn_array* dyn, pid_t pid) {
    for (int i = 0; i < dyn->size; i++) {
        trader* current = (trader*) dyn->array[i];
        if (current->pid == pid) {
            return current;
        }
    }
    return NULL;
}

product* dyn_array_get_product(dyn_array* dyn, char* name) {
    for (int i = 0; i < dyn->size; i++) {
        product* current = (product*) dyn->array[i];
        if (strncmp(current->name, name, strlen(current->name)) == 0) {
            return current;
        }
    }
    return NULL;
}

price_entry* dyn_array_get_price_entry(dyn_array* dyn, int value) {
    for (int i = 0; i < dyn->size; i++) {
        price_entry* current = (price_entry*) dyn_array_get(dyn, i);
        if (current->value == value) {
            return current;
        }
    }
    return NULL;
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
        trader* current = (trader*) dyn->array[i];
        // free trader attributes
        free(current->binary);
        free(current->exchange_pipe_path);
        free(current->trader_pipe_path);
        free(current->positions);
        // free traders themselves
        free(current);
    }
    return;
}

void dyn_array_delete_traders(dyn_array* dyn, int index) {
    int i = index;
    trader* current = (trader*) dyn_array_get(dyn, i);
    // free trader memory and clean up pipes
    // close(current->trader_pipe);
    // close(current->exchange_pipe);
    // unlink(current->trader_pipe_path);
    // unlink(current->exchange_pipe_path);
    free(current->binary);
    free(current->exchange_pipe_path);
    free(current->trader_pipe_path);
    free(current->positions); 
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
        // free name
        free(prod->name);
        // free buy_prices
        for (int j = 0; j < prod->buy_prices->size; j++) {
            price_entry* price = (price_entry*) prod->buy_prices->array[j];                        
            dyn_array_free_values(price->orders);
            dyn_array_free(price->orders);
            free(price);
        }
        dyn_array_free(prod->buy_prices);
        // free sell_prices
        for (int j = 0; j < prod->sell_prices->size; j++) {
            price_entry* price = (price_entry*) prod->sell_prices->array[j];                        
            dyn_array_free_values(price->orders);
            dyn_array_free(price->orders);
            free(price);
        }
        dyn_array_free(prod->sell_prices);
        // free product
        free(prod); 
    }
}

/* End of copied code */
