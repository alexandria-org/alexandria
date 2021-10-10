#!/bin/bash

apt-get update
apt-get -y install vim parted screen zip

_mkpart() { 
	disc=$1
	mountpoint1=$2
	mountpoint2=$3
	parted -s $disc mklabel gpt
	parted -s -a optimal $disc mkpart primary ext4 0% 50%
	parted -s -a optimal $disc mkpart primary ext4 50% 100%

	sleep 1

	mkfs.ext4 -F ${disc}p1
	mkfs.ext4 -F ${disc}p2

	mkdir $mountpoint1
	mount ${disc}p1 $mountpoint1

	mkdir $mountpoint2
	mount ${disc}p2 $mountpoint2
}

mkdir /mnt/0
mkdir /mnt/1

_mkpart /dev/nvme1n1 /mnt/2 /mnt/3
_mkpart /dev/nvme2n1 /mnt/4 /mnt/5
_mkpart /dev/nvme3n1 /mnt/6 /mnt/7

