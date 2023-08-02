## PEX
__Completed as a part of my COMP2017 assignment 2 at The University of Sydney__

### Description of the program

The requirements for this assignment involved creating an "exchange" program whose purpose was to execute and manage multiple "traders". The program was then tested against automated test cases where both the exchange and the trader, according to a list of products and their prices, host a trading simulation with multiple types of commands such as BUY, SELL, AMEND etc. The main concepts tested in this assignment were dynamic memory allocation and Inter-Process Communication using signals and pipes.

The exchange first reads the product file and makes a list of product structs,
which hold the product names and a list of unique price entries for buy and
sell orders. A list of trader structs is also made using the command line
arguments. The list of products essentially represents the order book. The buy
and sell price_entry structs store a list of order structs representing
individual orders. With this organisation, I was able to access the whole
order book in a systematic manner. For every new order, the exchange first
checks if it is valid. If it is, then the exchange stores it in the order book
and tries to match it with orders of the other type (buy with sell for
example). If there is a match, it is processed and the appropriate trader's
positions are updated, along with the order book itself depending on which order
has been fulfilled. All other order types such as AMEND and CANCEL are handled
accordingly, using the orderbook that is maintained.

### Important design decisions

My implementation of the auto-trader is relatively simple but fault-tolerant. In
the main event loop, the trader waits for a SIGUSR1 signal from the exchange,
upon which it reads the exchange pipe. If the read command is MARKET SELL, then
the trader sends a MARKET BUY request to the exchange by writing to the trader
pipe first, and then trying to send a SIGUSR1 signal. In order to ensure that the exchange does not miss the
trader's signal, the trader sends a SIGUSR1 every
2 seconds until it receives a SIGUSR1 response from the exchange. This ensures
that even if the exchange missed a signal sent by the trader, one signal will
be caught eventually. At this point, the trader reads the exchange pipe again
and repeats the process.
