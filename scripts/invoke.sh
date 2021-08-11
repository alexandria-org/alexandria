#!/bin/bash

cd `dirname $0`
cd ..
mkdir -p tmp

aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178359082.48/warc/CC-MAIN-20210227174711-20210227204711-00511.warc.gz"}' response.txt
#aws lambda invoke --function-name cc-api --payload '{"query":"pengar"}' tmp/response.txt

cat response.txt
echo ""
