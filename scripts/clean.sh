#!/bin/bash

cd `dirname $0`
cd ..

for shard in `cat shards`; do
	rm -r "$shard";
	mkdir "$shard";
	mkdir "$shard/input";
	mkdir "$shard/output";
	mkdir "$shard/upload";
done
