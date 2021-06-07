#!/bin/bash

cd `dirname $0`
cd ..

rm /mnt/fti_*

for shard in `cat shards`; do
	rm -r "$shard";
	mkdir "$shard";
	mkdir "$shard/input";
	mkdir "$shard/output";
	mkdir "$shard/upload";
	mkdir "$shard/hash_table";
done
