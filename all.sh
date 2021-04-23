#!/bin/bash

rm warc.paths.gz
wget https://commoncrawl.s3.amazonaws.com/crawl-data/CC-MAIN-2021-10/warc.paths.gz
gunzip -f warc.paths.gz

for key in `head -n 1000 warc.paths`; do

	aws lambda invoke --function-name cc-parser --invocation-type Event --payload '{"s3bucket":"commoncrawl", "s3key": "'"$key"'"}' response.txt

done

echo ""
