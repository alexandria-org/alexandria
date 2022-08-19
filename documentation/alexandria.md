Usage: ./alexandria [OPTIONS]...

## Options

**--downloader [commoncrawl-batch] [limit] [offset]**

Downloads files from the given commoncrawl batch. Limit and offset arguments are used for downloading a subset of the files. Example
```
./alexandria --downloader CC-MAIN-2022-27 2500 0
```
Will download the first 2500 files from CC-MAIN-2022-27 and upload them to the 'upload' host. See config documentation.

**--downloader-merge**

Merges downloaded files. This should run on the upload host to merge the different downloaded batches into our hash table.

**--hash-table-url [URL]**

Searches the local hash table called 'all_urls' for the given URL.

**--hash-table-url-hash [URL-hash]**

Searches the local hash table called 'all_urls' for the given URL-hash.

**--hash-table-count**

Counts all items in local hash table called 'all_urls'.

**--hash-table-find-all [HOST]**

Searches the local hash table called 'all_urls' for urls from specified host. This takes several days for large hash table.

**--hash-table-count [HOST]**

Estimated count of host from hash table by only counting one shard and multiply by number of shards.

**--hash-table-optimize-shard [SHARD]**

Optimizes shard for local hash table called 'all_urls'.

**--internal-harmonic**

Run the whole internal links harmonic calculator. Should run on 'upload' host.
