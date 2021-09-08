# COnfigure local nginx server.

1. Install nginx
```
apt-get install nginx
```

2. Add configuration to /etc/nginx/sites-available/default (If you are running other sites locally you should probably do something else here)
```
server {
	listen 80 default_server;
	listen [::]:80 default_server;

	root /var/www/html/node0003.alexandria.org;

	index index.html index.htm index.nginx-debian.html;

	server_name _;

	location / {
		try_files $uri $uri/ =404;
		autoindex on;
	}
}
```

3. Download test data to /var/www/html
```
./scripts/download-test-data.sh /var/www/html
```

