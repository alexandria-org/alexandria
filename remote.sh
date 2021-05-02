#!/bin/bash

cd `dirname $0`

IP=`aws ec2 describe-instances --query "Reservations[*].Instances[*].[PublicIpAddress]" --instance-ids i-0fe6ebd3fd03a7009 --output=text`

ssh -i /home/josef/alexandria-keys.pem ubuntu@$IP

