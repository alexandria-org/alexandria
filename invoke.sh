#!/bin/bash

aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl", "s3key": "crawl-data/CC-MAIN-2021-10/segments/1614178361510.12/warc/CC-MAIN-20210228145113-20210228175113-00135.warc.gz"}' response.txt
#aws lambda invoke --function-name cc-parser --payload '{"s3bucket":"commoncrawl-runtimes", "s3key": "example.txt.gz"}' response.txt
