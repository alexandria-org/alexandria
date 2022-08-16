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

**--hashtable-url [URL]**

Searches the local hash table called 'all_urls' for the given URL.

**--hashtable-url-hash [URL-hash]**

Searches the local hash table called 'all_urls' for the given URL-hash.

**--internal-harmonic**

Run the whole internal links harmonic calculator. Should run on 'upload' host.

