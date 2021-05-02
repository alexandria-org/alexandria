#!/bin/bash

#aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178359082.48/warc/CC-MAIN-20210227174711-20210227204711-00511.warc.gz"}' response.txt
aws lambda invoke --function-name cc-indexer --payload '{"s3bucket":"alexandria-cc-output", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178351134.11/warc/CC-MAIN-20210225124124-20210225154124-00003.warc.gz", "id": 123}' response.txt

cat response.txt
echo ""
