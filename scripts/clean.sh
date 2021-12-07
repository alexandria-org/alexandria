#!/bin/bash

cd `dirname $0`
cd ..

read -p "Do you want to delete your local alexandria data? [Y/n] " -n 1 -r
echo
if [[ $REPLY =~ ^[Y]$ ]]
then
	for shard in $(seq 0 7); do
		rm -r $shard/*
		mkdir $shard
		mkdir "$shard/input";
		mkdir "$shard/output";
		mkdir "$shard/upload";
		mkdir "$shard/hash_table";
		mkdir "$shard/full_text";
		mkdir "$shard/tmp";
	done

else
	echo "Ignoring"
fi

