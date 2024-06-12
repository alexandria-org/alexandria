#!/bin/bash

cd `dirname $0`

# The local docker development environment runs the data server on the local machine.
# This script sets that up and downloads the test data.

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
	location /store {
		fastcgi_pass   127.0.0.1:8001;
		fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
		fastcgi_param  SERVER_SOFTWARE    nginx;
		fastcgi_param  QUERY_STRING       \$query_string;
		fastcgi_param  REQUEST_METHOD     \$request_method;
		fastcgi_param  CONTENT_TYPE       \$content_type;
		fastcgi_param  CONTENT_LENGTH     \$content_length;
		fastcgi_param  SCRIPT_FILENAME    \$document_root\$fastcgi_script_name;
		fastcgi_param  SCRIPT_NAME        \$fastcgi_script_name;
		fastcgi_param  REQUEST_URI        \$request_uri;
		fastcgi_param  DOCUMENT_URI       \$document_uri;
		fastcgi_param  DOCUMENT_ROOT      \$document_root;
		fastcgi_param  SERVER_PROTOCOL    \$server_protocol;
		fastcgi_param  REMOTE_ADDR        \$remote_addr;
		fastcgi_param  REMOTE_PORT        \$remote_port;
		fastcgi_param  SERVER_ADDR        \$server_addr;
		fastcgi_param  SERVER_PORT        \$server_port;
		fastcgi_param  SERVER_NAME        \$server_name;
	}
}
" > /etc/nginx/sites-enabled/default

echo "Downloading test data";
./download-test-data.sh /var/www/html

mkdir /var/www/html/node0003.alexandria.org/nodes
mkdir /var/www/html/node0003.alexandria.org/nodes/test0001
mkdir /var/www/html/node0003.alexandria.org/upload-tmp

chown -R www-data:www-data /var/www/html/node0003.alexandria.org

/etc/init.d/nginx restart

./scripts/download-deps.sh
./scripts/build-deps.sh
