1. Describe how your exchange works.

The exchange first reads the product file and makes a list of product structs, which
hold the product names and a list for unique price entries for buy and sell orders. A list of trader structs
is also made using the command line arguments. The list of products essentially represents the orderbook.
The buy and sell price_entry structs store a list of order structs that represent individual orders. With this organisation,
I was able to access the whole orderbook in a systematic manner. For every new order, the exchange first checks if it is valid.
If it is, then the exchange stores it in the order book and tries to match it with orders of the other type 
(buy with sell for example). If there is a match, it is processed and the appropriate trader's positions are updated,
along with the orderbook itself depending on which order has been fulfilled. All other order types such as AMEND and CANCEL
are handled accordingly, using the orderbook that is maintained.

2. Describe your design decisions for the trader and how it's fault-tolerant.

3. Describe your tests and how to run them.

