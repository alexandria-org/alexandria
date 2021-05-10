#!/bin/bash

for shard in `ls /mnt`; do
	rm -r "/mnt/$shard";
	mkdir "/mnt/$shard";
	mkdir "/mnt/$shard/output";
	mkdir "/mnt/$shard/upload";
done
