#!/bin/bash

cd `dirname $0`
cd ..

for shard in $(seq 0 7); do
	rm -r /mnt/$shard/*
	mkdir "/mnt/$shard/input";
	mkdir "/mnt/$shard/output";
	mkdir "/mnt/$shard/upload";
	mkdir "/mnt/$shard/hash_table";
	mkdir "/mnt/$shard/full_text";
	mkdir "/mnt/$shard/tmp";
done

chown -R alexandria /mnt/*

