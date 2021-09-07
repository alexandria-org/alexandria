#!/bin/bash

cd `dirname $0`
cd ../build

make aws-lambda-package-cc_parser

aws lambda update-function-configuration --function-name cc-parser --handler cc_parser --runtime provided --role arn:aws:iam::259015057813:role/service-role/cc-parser-role-kqqwot3e
aws lambda update-function-code --function-name cc-parser --zip-file fileb://cc_parser.zip

