#include "functions.h"
#include "dyn_array.h"
#include "pe_common.h"

void match_order(exchange* pexchange, command command_type, char* product_name, int price, order* new_order, trader* source) {
    product* prod = (product*) dyn_array_get_product(pexchange->product_list, product_name);
    int prod_index;
    for (int i = 0; i < pexchange->product_list->size; i++) {
        product* current = (product*) dyn_array_get(pexchange->product_list, i);
        if (strncmp(current->name, prod->name, strlen(prod->name)) == 0) {
            prod_index = i;
            break;
        }
    }
    price_entry* prod_price;
    price_entry* current_price;
    switch (command_type) {
        case BUY:
            prod_price = (price_entry*) dyn_array_get_price_entry(prod->buy_prices, price);
            if (prod_price == NULL)
                printf("Something went super wrong here.\n");
            // loop through sell prices to check if there's any sell_price < buy_price
            for (int i = 0; i < prod->sell_prices->size; i++) { // prices are already sorted, so first match will be lowest price
                current_price = (price_entry*) dyn_array_get(prod->sell_prices, i);
                if (current_price->value <= price) {
                    // we have a match! (current price is the sell price)
                    // go through the orders at the current_price
                    // for (int j = 0; j < current_price->orders->size; j++) {
                    int j = 0;
                    while (j < current_price->orders->size) {
                        order* current_order = (order*) dyn_array_get(current_price->orders, j);
                        if (current_order->qty > new_order->qty) {
                            // take qty from current order and remove new_order
                            int value = current_price->value * new_order->qty;
                            int fee = round(0.01 * value);
                            // [PEX] Match: Order 3 [T0], New Order 1 [T1], value: $15060, fee: $151.
                            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
                                    LOG_PREFIX, current_order->order_id, current_order->source->id,
                                    new_order->order_id, new_order->source->id, value, fee);
                            // FILL ORDER
                            char* message;
                            // send to buyer
                            asprintf(&message, "FILL %d %d;", new_order->order_id, new_order->qty);
                            write(new_order->source->exchange_pipe, message, strlen(message));
                            kill(new_order->source->pid, SIGUSR1);
                            free(message);
                            // send to seller
                            asprintf(&message, "FILL %d %d;", current_order->order_id, new_order->qty);
                            write(current_order->source->exchange_pipe, message, strlen(message));
                            kill(current_order->source->pid, SIGUSR1);
                            free(message);
                            // update current_order
                            current_order->qty -= new_order->qty;
                            // update trader(s) values
                            new_order->source->positions[prod_index].qty += new_order->qty; // update qty
                            new_order->source->positions[prod_index].net_value -= value + fee; // update net_value
                            current_order->source->positions[prod_index].qty -= new_order->qty;
                            current_order->source->positions[prod_index].net_value += value;
                            // adding fee to exchange
                            pexchange->fee += fee;
                            // remove new_order from orderbook and free memory
                            free(dyn_array_get(prod_price->orders, prod_price->orders->size - 1));
                            dyn_array_delete(prod_price->orders, prod_price->orders->size - 1); // check free
                            return;
                        }
                        else if (current_order->qty == new_order->qty) {
                            // fulfill order and remove both current and new order
                            int value = current_price->value * new_order->qty;
                            int fee = round(0.01 * value);
                            // [PEX] Match: Order 3 [T0], New Order 1 [T1], value: $15060, fee: $151.
                            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
                                    LOG_PREFIX, current_order->order_id, current_order->source->id,
                                    new_order->order_id, new_order->source->id, value, fee);
                            // FILL ORDER
                            char* message;
                            // send to buyer
                            asprintf(&message, "FILL %d %d;", new_order->order_id, new_order->qty);
                            write(new_order->source->exchange_pipe, message, strlen(message));
                            kill(new_order->source->pid, SIGUSR1);
                            free(message);
                            // send to seller
                            asprintf(&message, "FILL %d %d;", current_order->order_id, new_order->qty);
                            write(current_order->source->exchange_pipe, message, strlen(message));
                            kill(current_order->source->pid, SIGUSR1);
                            free(message);
                            // update trader(s)
                            new_order->source->positions[prod_index].qty += new_order->qty; // update qty
                            new_order->source->positions[prod_index].net_value -= value + fee; // update net_value
                            current_order->source->positions[prod_index].qty -= new_order->qty;
                            current_order->source->positions[prod_index].net_value += value;
                            // adding fee to exchange
                            pexchange->fee += fee;
                            // remove both orders from orderbook
                            free(dyn_array_get(prod_price->orders, prod_price->orders->size - 1));
                            dyn_array_delete(prod_price->orders, prod_price->orders->size - 1); // check free
                            free(dyn_array_get(current_price->orders, j));
                            dyn_array_delete(current_price->orders, j); // check free
                            return;
                        }
                        else if (current_order->qty < new_order->qty) {
                            // take all qty from current order, remove current order and move onto next order in list
                            int value = current_price->value * current_order->qty;
                            int fee = round(0.01 * value);
                            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
                                    LOG_PREFIX, current_order->order_id, current_order->source->id,
                                    new_order->order_id, new_order->source->id, value, fee);
                            // FILL ORDER
                            char* message;
                            // send to buyer
                            asprintf(&message, "FILL %d %d;", new_order->order_id, current_order->qty);
                            write(new_order->source->exchange_pipe, message, strlen(message));
                            kill(new_order->source->pid, SIGUSR1);
                            free(message);
                            // send to seller
                            asprintf(&message, "FILL %d %d;", current_order->order_id, current_order->qty);
                            write(current_order->source->exchange_pipe, message, strlen(message));
                            kill(current_order->source->pid, SIGUSR1);
                            free(message);
                            // update new_order
                            new_order->qty -= current_order->qty; 
                            // update trader(s)
                            new_order->source->positions[prod_index].qty += current_order->qty;
                            new_order->source->positions[prod_index].net_value -= value + fee;
                            current_order->source->positions[prod_index].qty -= current_order->qty;
                            current_order->source->positions[prod_index].net_value += value;
                            // adding fee to exchange
                            pexchange->fee += fee;
                            // remove current_order from orerbook
                            free(dyn_array_get(current_price->orders, j));
                            dyn_array_delete(current_price->orders, j);
                            j--;
                            // continue to next order
                        }
                        j++;
                    }
                }
            }
            break;
        case SELL:
            prod_price = (price_entry*) dyn_array_get_price_entry(prod->sell_prices, price);
            // loop through buy prices to check if there's any buy_price > sell_price
            for (int i = prod->buy_prices->size - 1; i >= 0; i--) { // prices are sorted so loop from back to get highest price
                current_price = (price_entry*) dyn_array_get(prod->buy_prices, i);
                if (current_price->value >= price) {
                    // we have a match! (current price is the sell price)
                    // go through the orders at the current_price
                    // for (int j = 0; j < current_price->orders->size; j++) {
                    int j = 0;
                    while (j < current_price->orders->size) {
                        order* current_order = (order*) dyn_array_get(current_price->orders, j);
                        if (current_order->qty > new_order->qty) {
                            // take all qty from new_order and remove new_order
                            int value = current_price->value * new_order->qty;
                            int fee = round(0.01 * value);
                            // [PEX] Match: Order 3 [T0], New Order 1 [T1], value: $15060, fee: $151.
                            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
                                    LOG_PREFIX, current_order->order_id, current_order->source->id,
                                    new_order->order_id, new_order->source->id, value, fee);
                            // FILL ORDER
                            char* message;
                            // send to buyer
                            asprintf(&message, "FILL %d %d;", current_order->order_id, new_order->qty);
                            write(current_order->source->exchange_pipe, message, strlen(message));
                            kill(current_order->source->pid, SIGUSR1);
                            free(message);
                            // send to seller
                            asprintf(&message, "FILL %d %d;", new_order->order_id, new_order->qty);
                            write(new_order->source->exchange_pipe, message, strlen(message));
                            kill(new_order->source->pid, SIGUSR1);
                            free(message);
                            // update current_order
                            current_order->qty -= new_order->qty;
                            // update trader(s) values
                            new_order->source->positions[prod_index].qty -= new_order->qty; // update qty
                            new_order->source->positions[prod_index].net_value += value - fee; // update net_value
                            current_order->source->positions[prod_index].qty += new_order->qty;
                            current_order->source->positions[prod_index].net_value -= value;
                            // adding fee to exchange
                            pexchange->fee += fee;
                            // remove new_order from orderbook
                            free(dyn_array_get(prod_price->orders, prod_price->orders->size - 1));
                            dyn_array_delete(prod_price->orders, prod_price->orders->size - 1); // check free
                            return;
                        }
                        else if (current_order->qty == new_order->qty) {
                            // fulfill order and remove both current and new order
                            int value = current_price->value * new_order->qty;
                            int fee = round(0.01 * value);
                            // [PEX] Match: Order 3 [T0], New Order 1 [T1], value: $15060, fee: $151.
                            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
                                    LOG_PREFIX, current_order->order_id, current_order->source->id,
                                    new_order->order_id, new_order->source->id, value, fee);
                            // FILL ORDER
                            char* message;
                            // send to buyer
                            asprintf(&message, "FILL %d %d;", current_order->order_id, new_order->qty);
                            write(current_order->source->exchange_pipe, message, strlen(message));
                            kill(current_order->source->pid, SIGUSR1);
                            free(message);
                            // send to seller
                            asprintf(&message, "FILL %d %d;", new_order->order_id, new_order->qty);
                            write(new_order->source->exchange_pipe, message, strlen(message));
                            kill(new_order->source->pid, SIGUSR1);
                            free(message);
                            // update trader(s)
                            new_order->source->positions[prod_index].qty -= new_order->qty; // update qty
                            new_order->source->positions[prod_index].net_value += value - fee; // update net_value
                            current_order->source->positions[prod_index].qty += new_order->qty;
                            current_order->source->positions[prod_index].net_value -= value;
                            // adding fee to exchange
                            pexchange->fee += fee;
                            // remove both orders from orderbook
                            free(dyn_array_get(prod_price->orders, prod_price->orders->size - 1));
                            dyn_array_delete(prod_price->orders, prod_price->orders->size - 1); // check free
                            free(dyn_array_get(current_price->orders, j));
                            dyn_array_delete(current_price->orders, j); // check free
                            return;
                        }
                        else if (current_order->qty < new_order->qty) {
                            // take all qty from new order, remove current order and move onto next order in list
                            int value = current_price->value * current_order->qty;
                            int fee = round(0.01 * value);
                            printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
                                    LOG_PREFIX, current_order->order_id, current_order->source->id,
                                    new_order->order_id, new_order->source->id, value, fee);
                            // FILL ORDER
                            char* message;
                            // send to buyer
                            asprintf(&message, "FILL %d %d;", current_order->order_id, current_order->qty);
                            write(current_order->source->exchange_pipe, message, strlen(message));
                            kill(current_order->source->pid, SIGUSR1);
                            free(message);
                            // send to seller
                            asprintf(&message, "FILL %d %d;", new_order->order_id, current_order->qty);
                            write(new_order->source->exchange_pipe, message, strlen(message));
                            kill(new_order->source->pid, SIGUSR1);
                            free(message);
                            // update new_order
                            new_order->qty -= current_order->qty; 
                            // update trader(s)
                            new_order->source->positions[prod_index].qty -= current_order->qty;
                            new_order->source->positions[prod_index].net_value += value - fee;
                            current_order->source->positions[prod_index].qty += current_order->qty;
                            current_order->source->positions[prod_index].net_value -= value;
                            // adding fee to exchange
                            pexchange->fee += fee;
                            // remove current_order from orderbook
                            free(dyn_array_get(current_price->orders, j));
                            dyn_array_delete(current_price->orders, j); // check free
                            j--;
                            // continue to next order
                        }
                        j++;
                    }
                }
            }
            break;
    }
}

void clean_prices(exchange* pexchange) { // remove price entries that have 0 orders
    for (int i = 0; i < pexchange->product_list->size; i++) {
        product* prod = (product*) dyn_array_get(pexchange->product_list, i);
        for (int j = 0; j < prod->buy_prices->size; j++) {
            price_entry* price = (price_entry*) dyn_array_get(prod->buy_prices, j);
            if (price->orders->size == 0) {
                // remove price entry
                dyn_array_free(price->orders);
                free(price);
                dyn_array_delete(prod->buy_prices, j--);
            }
        }
        
        for (int j = 0; j < prod->sell_prices->size; j++) {
            price_entry* price = (price_entry*) dyn_array_get(prod->sell_prices, j);
            if (price->orders->size == 0) {
                // remove price entry
                dyn_array_free(price->orders);
                free(price);
                dyn_array_delete(prod->sell_prices, j--);
            }
        }
    }
}

order* store_product(exchange* pexchange, trader* source, command command_type, int order_id, char* product_name, int qty, int price) {
    // find the product the order is for
    product* prod = dyn_array_get_product(pexchange->product_list, product_name);
    // make a new order
    order* new_order = (order*) malloc(sizeof(order));
    new_order->qty = qty;
    new_order->order_id = order_id;
    new_order->source = source;
    // the price_entry if it exists
    price_entry* prod_price;
    switch (command_type) {
        case BUY:
            prod_price = dyn_array_get_price_entry(prod->buy_prices, price);
            if (prod_price == NULL) { // create a new price_entry
                price_entry* new_price = (price_entry*) malloc(sizeof(price_entry));
                new_price->value = price;
                new_price->orders = dyn_array_init();
                // add new_order to the new price_entry
                dyn_array_add(new_price->orders, (void*) new_order);
                // add the price to prices
                dyn_array_add_price(prod->buy_prices, (void*) new_price);
            }
            else { // means price_entry exists
                dyn_array_add(prod_price->orders, (void*) new_order);
            }
            break;
        case SELL:
            prod_price = dyn_array_get_price_entry(prod->sell_prices, price);
            if (prod_price == NULL) { // create a new price_entry
                price_entry* new_price = (price_entry*) malloc(sizeof(price_entry));
                new_price->value = price;
                new_price->orders = dyn_array_init();
                // add new_order to the new price_entry
                dyn_array_add(new_price->orders, (void*) new_order);
                // add the price to prices
                dyn_array_add_price(prod->sell_prices, (void*) new_price);
            }
            else { // means price_entry exists
                dyn_array_add(prod_price->orders, (void*) new_order);
            }
            break;
    }
    return new_order;
}

void print_report(exchange* pexchange) {
    clean_prices(pexchange);
    printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);
    // print product information
    for (int i = 0; i < pexchange->product_list->size; i++) {
        product* prod = (product*) dyn_array_get(pexchange->product_list, i);
        printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, prod->name, prod->buy_prices->size, prod->sell_prices->size);
        // print sell levels
        for (int j = prod->sell_prices->size-1; j >= 0; j--) {
            price_entry* price = (price_entry*) dyn_array_get(prod->sell_prices, j); 
            // print every price level
            int qty = 0;
            for (int k = 0; k < price->orders->size; k++) {
                order* ord = (order*) dyn_array_get(price->orders, k);
                qty += ord->qty;
            }
            if (price->orders->size == 0)
                continue;
            else if (price->orders->size == 1)
                printf("%s\t\tSELL %d @ $%d (%d order)\n", LOG_PREFIX, qty, price->value, price->orders->size);
            else
                printf("%s\t\tSELL %d @ $%d (%d orders)\n", LOG_PREFIX, qty, price->value, price->orders->size);
        }
        // print buy levels
        for (int j = prod->buy_prices->size-1; j >= 0; j--) {
            price_entry* price = (price_entry*) dyn_array_get(prod->buy_prices, j); 
            // print every price level
            int qty = 0;
            for (int k = 0; k < price->orders->size; k++) {
                order* ord = (order*) dyn_array_get(price->orders, k);
                qty += ord->qty;
            }
            if (price->orders->size == 0)
                continue;
            else if (price->orders->size == 1)
                printf("%s\t\tBUY %d @ $%d (%d order)\n", LOG_PREFIX, qty, price->value, price->orders->size);
            else
                printf("%s\t\tBUY %d @ $%d (%d orders)\n", LOG_PREFIX, qty, price->value, price->orders->size);
        }
    }
    // print positions
    printf("%s\t--POSITIONS--\n", LOG_PREFIX);
    for (int i = 0; i < pexchange->traders->size; i++) {
        trader* current = (trader*) dyn_array_get(pexchange->traders, i);
        printf("%s\tTrader %d: ", LOG_PREFIX, current->id);
        // for every trader, go through all products and print positions
        for (int j = 0; j < pexchange->product_list->size; j++) {
            product* prod = (product*) dyn_array_get(pexchange->product_list, j);
            if (j == 0)
                printf("%s %d ($%d)", prod->name, current->positions[j].qty, current->positions[j].net_value);
            else
                printf(", %s %d ($%d)", prod->name, current->positions[j].qty, current->positions[j].net_value);
        }
        printf("\n");
    }
}

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
        if (current->connected == false)
            continue;
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
    new_trader->positions = (position*) calloc(pexchange->num_products, sizeof(position));

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
    new_trader->connected = true;
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
        printf("%s Connected to %s\n", LOG_PREFIX, new_trader->exchange_pipe_path);
    }
    if ((new_trader->trader_pipe = open(new_trader->trader_pipe_path, O_RDONLY)) == -1) {
        printf("\nTrader id: %d\n", i);
        perror("Error opening trader_pipe");
    }
    else {
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
