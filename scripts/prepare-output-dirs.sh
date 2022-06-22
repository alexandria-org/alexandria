#!/bin/bash

cd `dirname $0`
cd ..

for shard_id in $(seq 0 7); do
	shard="/mnt/$shard_id"
	rm -r $shard
	mkdir $shard
	mkdir "$shard/input";
	mkdir "$shard/output";
	mkdir "$shard/upload";
	mkdir "$shard/hash_table";
	mkdir "$shard/full_text";
	mkdir "$shard/tmp";
done
