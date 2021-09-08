#!/bin/bash

cd `dirname $0`

echo "Copying nginx config";

echo "server {
	listen 80 default_server;
	listen [::]:80 default_server;

	root /var/www/html/node0003.alexandria.org;
	index index.html;
	server_name _;

	location / {
		try_files \$uri \$uri/ =404;
	}
}
" > /etc/nginx/sites-available/default 

/etc/init.d/nginx restart

echo "Downloading test data";
./download-test-data.sh /var/www/html

for shard in `cat ../shards`; do
	rm -r $shard/*
	mkdir $shard
	mkdir "$shard/input";
	mkdir "$shard/output";
	mkdir "$shard/upload";
	mkdir "$shard/hash_table";
	mkdir "$shard/full_text";
	mkdir "$shard/tmp";
done

