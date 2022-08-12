# The alexandria full text index

A full text index in its simplest form is a hash map from an integer word id k to a list of document ids v.

We built two kinds of data structures called ```index``` and ```counted_index```. Both data structures acts on a given template type
```data_record```.
They are very similar so we will start by explaining the similar parts and then explain the differences and why we needed two.

## Data layout

One index starts with a hash table. The hash table contains the position of the page that contains all the keys mapped to that specific hash position.
```
hash table        : uint64_t[hash_table_size] (8 x hash_table_size bytes)
num_records       : uint64_t (8 bytes)
list of records   : data_record[num_records] (sizeof(data_record) * num_records bytes)
consecutive pages : page[varying] (undetermined size)
```

A single page consists of a list of keys. Each key then has a corresponding position among the bitmaps and a length of the bitmap. The bitmaps (of varying length) are then stored consecutively.
```
num_keys             : uint64_t (8 bytes)
list of keys         : uint64_t[num_keys] (8 x num_keys bytes)
list of positions    : uint64_t[num_keys] (8 x num_keys bytes)
list of lengths      : uint64_t[num_keys] (8 x num_keys bytes)
consecutive bitmaps  : bitmap[num_keys] (undetermined size)
```



