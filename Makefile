# CC=gcc
# CFLAGS=-Wall -Werror -Wvla -O0 -std=gnu99 -g -fsanitize=address,leak
# LDFLAGS=-lm
# BINARIES=pe_exchange pe_trader dyn_array
# 
# # CFLAGS 	   = -O0 -flto -std=c11 -pie -Oz -lm
# SRC=pe_exchange.c pe_trader.c dyn_array.c
# 
# all:$(BINARIES)
# 
# $(TARGET):pe_exchange
# 	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o pe_exchange
# 
# .PHONY: clean
# clean:
# 	rm -f $(BINARIES)

CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
LDFLAGS=-lm -fsanitize=address,leak
TARGET=pe_exchange pe_trader

.PHONY: clean
all: $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o

pe_exchange.o: pe_exchange.c
	$(CC) -c $(CFLAGS) $^ -o $@

dyn_array.o: dyn_array.c
	$(CC) -c $(CFLAGS) $^ -o $@

functions.o: functions.c
	$(CC) -c $(CFLAGS) $^ -o $@

pe_trader.o: pe_trader.c
	$(CC) -c $(CFLAGS) $^ -o $@

pe_exchange: pe_exchange.o dyn_array.o functions.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

pe_trader: pe_trader.o dyn_array.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
