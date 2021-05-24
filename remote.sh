#!/bin/bash

cd `dirname $0`

INSTANCE_ID=`cat instance.txt`

IP=`aws ec2 describe-instances --query "Reservations[*].Instances[*].[PublicIpAddress]" --instance-ids $INSTANCE_ID --output=text`

ssh -i /home/josef/alexandria-keys.pem ubuntu@$IP

