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

sudo mkdir -p /mnt/00
sudo mkdir -p /mnt/01
sudo mkdir -p /mnt/02
sudo mkdir -p /mnt/03
sudo mkdir -p /mnt/04
sudo mkdir -p /mnt/05
sudo mkdir -p /mnt/06
sudo mkdir -p /mnt/07

sudo mount /dev/nvme0n1 /mnt/00
sudo mount /dev/nvme1n1 /mnt/01
sudo mount /dev/nvme2n1 /mnt/02
sudo mount /dev/nvme3n1 /mnt/03
sudo mount /dev/nvme4n1 /mnt/04
sudo mount /dev/nvme5n1 /mnt/05
sudo mount /dev/nvme6n1 /mnt/06
sudo mount /dev/nvme7n1 /mnt/07

EOF

echo "ssh -i /home/josef/alexandria-keys.pem -o StrictHostKeyChecking=no ubuntu@$IP"

