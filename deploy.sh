#!/bin/bash

cd `dirname $0`

INSTANCE_ID=`cat instance.txt`

make cc_api

make aws-lambda-package-cc_parser
make aws-lambda-package-cc_indexer
make aws-lambda-package-cc_api

aws lambda update-function-configuration --function-name cc-parser --handler cc_parser --runtime provided --role arn:aws:iam::259015057813:role/service-role/cc-parser-role-kqqwot3e
aws lambda update-function-code --function-name cc-parser --zip-file fileb://cc_parser.zip

aws lambda update-function-configuration --function-name cc-api --handler cc_api --runtime provided --role arn:aws:iam::259015057813:role/service-role/cc-parser-role-kqqwot3e
aws lambda update-function-code --function-name cc-api --zip-file fileb://cc_api.zip

IP=`aws ec2 describe-instances --query "Reservations[*].Instances[*].[PublicIpAddress]" --instance-ids $INSTANCE_ID --output=text`

scp -i /home/josef/alexandria-keys.pem ./cc_indexer.zip ubuntu@$IP:/home/ubuntu/
ssh -i /home/josef/alexandria-keys.pem ubuntu@$IP 'unzip -o cc_indexer.zip'

#sudo shutdown -h now
