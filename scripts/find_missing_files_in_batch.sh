#!/bin/bash

cd `dirname $0`
cd ..

batch=$1

files=`curl https://data.commoncrawl.org/crawl-data/$batch/warc.paths.gz | gunzip`

for raw_file in $files; do
	file="${raw_file/.warc.gz/.gz}"
	if [[ -f "$file" ]]; then
		filesize=$(stat -c%s "$file")
		if [[ $filesize -lt 1000 ]]; then
			echo "The file '$file' exists and is small."
		fi
	fi
done


