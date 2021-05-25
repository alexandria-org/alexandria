#!/bin/bash

cd `dirname $0`
cd ..

INSTANCE_ID=`cat instance.txt`

aws ec2 stop-instances --instance-ids $INSTANCE_ID
aws ec2 wait instance-stopped --instance-ids $INSTANCE_ID

