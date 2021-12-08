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
			autoindex on;
    		client_body_temp_path /var/www/html/node0003.alexandria.org/upload-tmp;
    		dav_methods PUT;
    		create_full_put_path  on;
    		dav_access group:rw  all:r;
    		client_max_body_size 10000m;
	}
}
" > /etc/nginx/sites-available/default

/etc/init.d/nginx restart

echo "Downloading test data";
./download-test-data.sh /var/www/html

mkdir /var/www/html/node0003.alexandria.org/upload
mkdir /var/www/html/node0003.alexandria.org/upload-tmp

chown -R www-data:www-data /var/www/html/node0003.alexandria.org

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

