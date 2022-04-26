### NAME

indexer - manually index data or analyze things

### SYNOPSIS

indexer [OPTION]

### DESCRIPTION
```
	--split source_batch target_prefix
		splits the urls in the local source batch and outputs them into {target_prefix}-[0-23]/files.
		for example --split CC-MAIN-2021-04 /mnt/crawl-data/NODE
	--split-count
	--split-count-domains
	--split-count-links
	--split-make-scraper-urls

	--tools-download-batch
	--tools-upload-urls-with-links
	--tools-find-links

	--calculate-harmonic-hosts
	--calculate-harmonic-links
	--calculate-harmonic

	--host-hash
	--host-hash-mod

	--console
		run the interactive console for making debug searches.

	--index-domans BATCH LIMIT OFFSET
		run the indexer for our domain index adding the urls+data from BATCH
	--index-links BATCH LIMIT OFFSET
		run the link indexer adding url_ and domain_ links from BATCH
	--index-words BATCH LIMIT OFFSET
		run the word indexer adding word data from BATCH
	--index-urls BATCH LIMIT OFFSET
		run the url indexer on batch generating one index per domain
	--index-snippets BATCH LIMIT OFFSET
		run the snippet indexer

	--truncate-domains
	--truncate-links
	--truncate-words
	--truncate-urls
	--truncate-snippets

	--info
		print info about indexes
```
