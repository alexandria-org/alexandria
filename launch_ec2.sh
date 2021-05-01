#!/bin/bash

sudo mkfs -t xfs /dev/nvme0n1
sudo mkfs -t xfs /dev/nvme1n1
sudo mkfs -t xfs /dev/nvme2n1
sudo mkfs -t xfs /dev/nvme3n1
sudo mkfs -t xfs /dev/nvme4n1
sudo mkfs -t xfs /dev/nvme5n1
sudo mkfs -t xfs /dev/nvme6n1
sudo mkfs -t xfs /dev/nvme7n1

mkdir /mnt/00
mkdir /mnt/01
mkdir /mnt/02
mkdir /mnt/03
mkdir /mnt/04
mkdir /mnt/05
mkdir /mnt/06
mkdir /mnt/07

sudo mount /dev/nvme0n1 /mnt/00
sudo mount /dev/nvme1n1 /mnt/01
sudo mount /dev/nvme2n1 /mnt/02
sudo mount /dev/nvme3n1 /mnt/03
sudo mount /dev/nvme4n1 /mnt/04
sudo mount /dev/nvme5n1 /mnt/05
sudo mount /dev/nvme6n1 /mnt/06
sudo mount /dev/nvme7n1 /mnt/07


