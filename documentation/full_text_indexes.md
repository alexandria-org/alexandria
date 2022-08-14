# The alexandria full text index

A full text index in its simplest form is a hash map from an integer word id ```key``` to a list of documents.

There are two kinds of data structures called ```index``` and ```counted_index```. Both data structures acts on a given template type
```data_record```.
The two data structures shares the same data layout except for the last part where ```index``` stores roaring bitmaps while `counted_index` store the records.

## Data layout

The index starts with a hash table. The hash table stores the position for the page containing `key` at index `key % hash_table_size`.

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



