#!/bin/bash

cd `dirname $0`

make aws-lambda-package-cc_parser

aws lambda update-function-configuration --function-name cc-parser --handler cc_parser --runtime provided --role arn:aws:iam::011852242707:role/pywren_exec_role_1
aws lambda update-function-code --function-name cc-parser --zip-file fileb://cc_parser.zip


