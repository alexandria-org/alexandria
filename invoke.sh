#!/bin/bash

aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178359082.48/warc/CC-MAIN-20210227174711-20210227204711-00511.warc.gz"}' response.txt
#aws lambda invoke --function-name cc-indexer --payload '{"s3bucket":"commoncrawl-output", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178347293.1/warc/CC-MAIN-20210224165708-20210224195708-00008.warc.gz"}' response.txt

cat response.txt
echo ""
