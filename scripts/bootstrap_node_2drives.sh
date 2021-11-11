#!/bin/bash

apt-get update
apt-get -y install vim parted zip unzip nginx

echo "\nALEXANDRIA_LIVE=1" >> /etc/environment

_mkpart() { 
	disc=$1
	mountpoint1=$2
	mountpoint2=$3
	mountpoint3=$4
	mountpoint4=$5
	parted -s $disc mklabel gpt
	parted -s -a optimal $disc mkpart primary ext4 0% 25%
	parted -s -a optimal $disc mkpart primary ext4 25% 50%
	parted -s -a optimal $disc mkpart primary ext4 50% 75%
	parted -s -a optimal $disc mkpart primary ext4 75% 100%

	sleep 1

	mkfs.ext4 -F ${disc}p1
	mkfs.ext4 -F ${disc}p2
	mkfs.ext4 -F ${disc}p3
	mkfs.ext4 -F ${disc}p4

	mkdir $mountpoint1
	mkdir $mountpoint2
	mkdir $mountpoint3
	mkdir $mountpoint4

	mount ${disc}p1 $mountpoint1
	mount ${disc}p2 $mountpoint2
	mount ${disc}p3 $mountpoint3
	mount ${disc}p4 $mountpoint4

	echo "" >> /etc/fstab
	echo "${disc}p1 $mountpoint1 ext4 noatime,nodiratime,barrier=0 0 0" >> /etc/fstab
	echo "${disc}p2 $mountpoint2 ext4 noatime,nodiratime,barrier=0 0 0" >> /etc/fstab
	echo "${disc}p3 $mountpoint3 ext4 noatime,nodiratime,barrier=0 0 0" >> /etc/fstab
	echo "${disc}p4 $mountpoint4 ext4 noatime,nodiratime,barrier=0 0 0" >> /etc/fstab
}

mkdir /mnt/0
mkdir /mnt/1
mkdir /mnt/2
mkdir /mnt/3

_mkpart /dev/nvme1n1 /mnt/4 /mnt/5 /mnt/6 /mnt/7

for shard in $(seq 0 7); do
	mkdir "/mnt/$shard/input";
	mkdir "/mnt/$shard/output";
	mkdir "/mnt/$shard/upload";
	mkdir "/mnt/$shard/hash_table";
	mkdir "/mnt/$shard/full_text";
	mkdir "/mnt/$shard/tmp";
done

echo "server {
    listen 80;
    server_name localhost;

    location / {
        fastcgi_pass   127.0.0.1:8000;
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
}" > /etc/nginx/sites-enabled/default
/etc/init.d/nginx restart

adduser --system --shell /sbin/nologin --gecos "User for running alexandria service" --disabled-password --home /alexandria alexandria

touch /var/log/alexandria.log
chown alexandria:syslog /var/log/alexandria.log

echo "
[Unit]
Description=Alexandria Server

[Service]
User=alexandria
WorkingDirectory=/alexandria
ExecStart=/alexandria/server
Restart=always

[Install]
WantedBy=multi-user.target
"

> /etc/systemd/system/alexandria.service


