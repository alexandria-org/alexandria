# Index file format

```8 bytes number of keys (n)
8 * n bytes keys
8 * n bytes positions
8 * n bytes lengths (len(k) number of records for key k)
[Data Records]
```

```
Data records are structured like this:
8 bytes total number of results
len(k) * (8 bytes unsigned long URL id, 4 bytes unsigned integer score)
