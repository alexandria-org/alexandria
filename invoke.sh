#!/bin/bash

#aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178389798.91/warc/CC-MAIN-20210309092230-20210309122230-00583.warc.gz"}' response.txt
aws lambda invoke --function-name cc-parser --invocation-type Event --payload '{"s3bucket":"commoncrawl", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178366959.54/warc/CC-MAIN-20210303104028-20210303134028-00435.warc.gz"}' response.txt
#aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl-runtimes", "s3key": "example.txt.gz"}' response.txt

cat response.txt
echo ""
