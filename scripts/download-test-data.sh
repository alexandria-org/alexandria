#!/bin/bash

cd `dirname $0`

if [ $# -eq 0 ]; then
	echo "Provide destination path as first argument"
	exit 1
fi

DEST=$1

cd $DEST || { echo "target directory does not exist"; exit 127; }

rm -r node0003.alexandria.org
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-01/ --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-02/ --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-03/ --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-04/ --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-TEST-05/ --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-MANUAL-01/warc.paths.gz --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/crawl-data/ALEXANDRIA-MANUAL-01/files/top_domains.txt.gz --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/dev_files/ --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/example.txt --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/example.txt.gz --http-user=alexandria --http-password=wmXN6U4u
wget -r --no-parent http://node0003.alexandria.org/test-data/ --http-user=alexandria --http-password=wmXN6U4u

