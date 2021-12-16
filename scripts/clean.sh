#!/bin/bash

cd `dirname $0`
cd ..

read -p "Do you want to delete your local alexandria data? [Y/n] " -n 1 -r
echo
if [[ $REPLY =~ ^[Y]$ ]]
then
	for shard in $(seq 0 7); do
		rm -r /mnt/$shard/*
		mkdir /mnt/$shard
		mkdir "/mnt/$shard/input";
		mkdir "/mnt/$shard/output";
		mkdir "/mnt/$shard/upload";
		mkdir "/mnt/$shard/hash_table";
		mkdir "/mnt/$shard/full_text";
		mkdir "/mnt/$shard/tmp";
	done

else
	echo "Ignoring"
fi

