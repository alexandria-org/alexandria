# Index file format

```8 bytes number of keys (n)
8 * n bytes keys
8 * n bytes positions
8 * n bytes lengths (len(k) number of records for key k)
8 * n bytes total found results
[Data Records]
```

```
Data records are structured like this:
len(k) * (8 bytes unsigned long URL id, 4 bytes single precision float score)
