#!/bin/bash

cd `dirname $0`
cd ..

for shard in `ls /mnt`; do
	rm -r "/mnt/$shard";
	mkdir "/mnt/$shard";
	mkdir "/mnt/$shard/input";
	mkdir "/mnt/$shard/output";
	mkdir "/mnt/$shard/upload";
done
