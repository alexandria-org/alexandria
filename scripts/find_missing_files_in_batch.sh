#!/bin/bash

cd `dirname $0`
cd ..

batch=$1

files=`curl https://data.commoncrawl.org/crawl-data/$batch/warc.paths.gz | gunzip`

missing_files_path="/mnt/crawl-data/$batch/missing.paths"

truncate -s 0 $missing_files_path

for raw_file in $files; do
	file="/mnt/${raw_file/.warc.gz/.gz}"
	if [[ -f "$file" ]]; then
		filesize=$(stat -c%s "$file")
		if [[ $filesize -lt 1000 ]]; then
			echo "The file '$file' exists and is small."
			echo $raw_file >> $missing_files_path
		fi
	fi
done

gzip $missing_files_path

