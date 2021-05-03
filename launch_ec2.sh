#!/bin/bash

aws ec2 start-instances --instance-ids i-0fe6ebd3fd03a7009
aws ec2 wait instance-running --instance-ids i-0fe6ebd3fd03a7009
aws ec2 wait instance-status-ok --instance-ids i-0fe6ebd3fd03a7009

IP=`aws ec2 describe-instances --query "Reservations[*].Instances[*].[PublicIpAddress]" --instance-ids i-0fe6ebd3fd03a7009 --output=text`

ssh -i /home/josef/alexandria-keys.pem -o StrictHostKeyChecking=no ubuntu@$IP << EOF

sudo mkfs -t xfs /dev/nvme0n1
sudo mkfs -t xfs /dev/nvme1n1
sudo mkfs -t xfs /dev/nvme2n1
sudo mkfs -t xfs /dev/nvme3n1
sudo mkfs -t xfs /dev/nvme4n1
sudo mkfs -t xfs /dev/nvme5n1
sudo mkfs -t xfs /dev/nvme6n1
sudo mkfs -t xfs /dev/nvme7n1

sudo mkdir -p /mnt/0
sudo mkdir -p /mnt/1
sudo mkdir -p /mnt/2
sudo mkdir -p /mnt/3
sudo mkdir -p /mnt/4
sudo mkdir -p /mnt/5
sudo mkdir -p /mnt/6
sudo mkdir -p /mnt/7

sudo mount /dev/nvme0n1 /mnt/0
sudo mount /dev/nvme1n1 /mnt/1
sudo mount /dev/nvme2n1 /mnt/2
sudo mount /dev/nvme3n1 /mnt/3
sudo mount /dev/nvme4n1 /mnt/4
sudo mount /dev/nvme5n1 /mnt/5
sudo mount /dev/nvme6n1 /mnt/6
sudo mount /dev/nvme7n1 /mnt/7

EOF

echo "ssh -i /home/josef/alexandria-keys.pem -o StrictHostKeyChecking=no ubuntu@$IP"

