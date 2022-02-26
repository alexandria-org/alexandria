#!/bin/bash

cd `dirname $0`

if [ $# -eq 0 ]; then
	echo "Provide destination path as first argument"
	exit 1
fi

for shard in $(seq 0 7); do
	mkdir "/mnt/$shard";
	mkdir "/mnt/$shard/input";
	mkdir "/mnt/$shard/output";
	mkdir "/mnt/$shard/upload";
	mkdir "/mnt/$shard/hash_table";
	mkdir "/mnt/$shard/full_text";
	mkdir "/mnt/$shard/tmp";
done

DEST=$1

cd $DEST || { echo "target directory does not exist"; exit 127; }

rm -r node0003.alexandria.org
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-01/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-02/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-03/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-04/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-05/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-06/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-07/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-08/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-09/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-10/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-MANUAL-01/warc.paths.gz --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-MANUAL-01/files/top_domains.txt.gz --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/dev_files/ --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/example.txt --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/example.txt.gz --http-user=alexandria --http-password=wmXN6U4u
wget -r -l1 --no-parent http://node0003.alexandria.org/test-data/ --http-user=alexandria --http-password=wmXN6U4u

mkdir node0003.alexandria.org/nodes
mkdir node0003.alexandria.org/nodes/test0001
mkdir node0003.alexandria.org/upload-tmp

chown -R www-data:www-data node0003.alexandria.org

