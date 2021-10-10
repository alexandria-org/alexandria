## Performance journal

2021-10-06, AX61-NVME with two discs
```
Server Software:        nginx/1.18.0
Server Hostname:        node0002.alexandria.org
Server Port:            80

Concurrency Level:      5
Time taken for tests:   294.451 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      294262066 bytes
HTML transferred:       293986342 bytes
Requests per second:    6.79 [#/sec] (mean)
Time per request:       736.127 [ms] (mean)
Time per request:       147.225 [ms] (mean, across all concurrent requests)
Transfer rate:          975.94 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       12   19  10.1     16     152
Processing:    16  717 461.5    652    2896
Waiting:        0  662 431.7    587    2770
Total:         31  736 460.4    671    2911

Percentage of the requests served within a certain time (ms)
  50%    671
  66%    879
  75%   1009
  80%   1108
  90%   1344
  95%   1595
  98%   1864
  99%   2062
 100%   2911 (longest request)
```

2021-10-10, AX41-NVMe with four discs
```
Server Software:        nginx/1.18.0
Server Hostname:        65.21.238.146
Server Port:            80

Concurrency Level:      5
Time taken for tests:   278.694 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      232745432 bytes
HTML transferred:       232469432 bytes
Requests per second:    7.18 [#/sec] (mean)
Time per request:       696.735 [ms] (mean)
Time per request:       139.347 [ms] (mean, across all concurrent requests)
Transfer rate:          815.56 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       12   69  98.4     35    1107
Processing:    14  627 698.4    454    9790
Waiting:       14  435 346.5    368    4045
Total:         29  696 719.1    522   10159

Percentage of the requests served within a certain time (ms)
  50%    522
  66%    755
  75%    927
  80%   1050
  90%   1382
  95%   1781
  98%   2415
  99%   3439
 100%  10159 (longest request)
```
