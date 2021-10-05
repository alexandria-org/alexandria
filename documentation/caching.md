## Caching

Our nodes should try to use as much RAM as possible to store index data for common tokens in RAM. I think the best way would be to hold a list of the most commonly queried tokens.

We can use /proc/meminfo to retrieve information about available memory on the server.
