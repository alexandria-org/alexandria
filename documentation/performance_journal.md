## Performance journal

### File system testing
Ext2 (noatime,nodiratime,barrier=0)
```
$ dd if=/dev/zero of=/tmp/test1.img bs=10G count=1 oflag=dsync
0+1 records in
0+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 4.76649 s, 451 MB/s

$ echo 3 > /proc/sys/vm/drop_caches

$ time dd if=/tmp/test1.img of=/dev/null bs=8k
262143+1 records in
262143+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 1.43043 s, 1.5 GB/s

real	0m1.435s
user	0m0.013s
sys	0m0.763s
```
Ext2 (relatime)
```
$ dd if=/dev/zero of=/tmp/test1.img bs=10G count=1 oflag=dsync
0+1 records in
0+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 5.02563 s, 427 MB/s

$ echo 3 > /proc/sys/vm/drop_caches

$ time dd if=/tmp/test1.img of=/dev/null bs=8k
262143+1 records in
262143+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 1.48533 s, 1.4 GB/s

real	0m1.490s
user	0m0.046s
sys	0m0.604s
```

Ext4 (noatime,nodiratime,barrier=0):
```
$ dd if=/dev/zero of=/tmp/test1.img bs=10G count=1 oflag=dsync
0+1 records in
0+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 2.26469 s, 948 MB/s

$ echo 3 > /proc/sys/vm/drop_caches

$ time dd if=/tmp/test1.img of=/dev/null bs=8k
262143+1 records in
262143+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 0.821499 s, 2.6 GB/s

real	0m0.824s
user	0m0.004s
sys	0m0.648s
```

Ext4 (relatime):
```
$ dd if=/dev/zero of=/tmp/test1.img bs=10G count=1 oflag=dsync
0+1 records in
0+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 2.15461 s, 997 MB/s

$ echo 3 > /proc/sys/vm/drop_caches

$ time dd if=/tmp/test1.img of=/dev/null bs=8k
262143+1 records in
262143+1 records out
2147479552 bytes (2.1 GB, 2.0 GiB) copied, 0.822013 s, 2.6 GB/s

real	0m0.825s
user	0m0.029s
sys	0m0.568s
```

Conclusion. Run ext4

### Software load testing
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

2021-10-10, AX61-NVME with two discs
```
Server Software:        nginx/1.18.0
Server Hostname:        node0002.alexandria.org
Server Port:            80

Concurrency Level:      5
Time taken for tests:   328.051 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      255881934 bytes
HTML transferred:       255605934 bytes
Requests per second:    6.10 [#/sec] (mean)
Time per request:       820.128 [ms] (mean)
Time per request:       164.026 [ms] (mean, across all concurrent requests)
Transfer rate:          761.73 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       12   52  95.6     25    1560
Processing:    16  767 558.9    689    3961
Waiting:       15  638 427.9    594    2631
Total:         32  819 558.5    742    4113

Percentage of the requests served within a certain time (ms)
  50%    742
  66%    982
  75%   1159
  80%   1260
  90%   1560
  95%   1831
  98%   2186
  99%   2470
 100%   4113 (longest request)
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

2021-10-10, AX41-NVMe with four discs
```
Server Software:        nginx/1.18.0
Server Hostname:        65.21.238.146
Server Port:            80

Concurrency Level:      5
Time taken for tests:   252.503 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      230349918 bytes
HTML transferred:       230073780 bytes
Requests per second:    7.92 [#/sec] (mean)
Time per request:       631.258 [ms] (mean)
Time per request:       126.252 [ms] (mean, across all concurrent requests)
Transfer rate:          890.88 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       12   54  78.2     27    1068
Processing:    15  576 519.3    436    3659
Waiting:       15  421 325.7    354    2421
Total:         30  631 527.6    491    3728

Percentage of the requests served within a certain time (ms)
  50%    491
  66%    707
  75%    861
  80%    988
  90%   1355
  95%   1736
  98%   2100
  99%   2419
 100%   3728 (longest request)
```

2021-10-10, AX61-NVME with two discs, 4 partitions
```
Server Software:        nginx/1.18.0
Server Hostname:        65.21.125.158
Server Port:            80

Concurrency Level:      5
Time taken for tests:   263.283 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      282821583 bytes
HTML transferred:       282545445 bytes
Requests per second:    7.60 [#/sec] (mean)
Time per request:       658.209 [ms] (mean)
Time per request:       131.642 [ms] (mean, across all concurrent requests)
Transfer rate:          1049.03 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       13   28  32.9     26     630
Processing:    17  629 434.1    563    3051
Waiting:       15  587 412.8    517    2949
Total:         36  657 435.8    593    3090

Percentage of the requests served within a certain time (ms)
  50%    593
  66%    774
  75%    914
  80%   1003
  90%   1260
  95%   1480
  98%   1708
  99%   1959
 100%   3090 (longest request)
```

2021-10-10, AX61-NVME with two discs, 4 partitions
```
Server Software:        nginx/1.18.0
Server Hostname:        65.21.125.158
Server Port:            80

Concurrency Level:      5
Time taken for tests:   249.241 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      267058842 bytes
HTML transferred:       266782842 bytes
Requests per second:    8.02 [#/sec] (mean)
Time per request:       623.101 [ms] (mean)
Time per request:       124.620 [ms] (mean, across all concurrent requests)
Transfer rate:          1046.38 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       13   27  19.3     25     734
Processing:    15  596 469.4    506    3785
Waiting:        0  554 449.3    467    3660
Total:         32  622 470.7    531    3805

Percentage of the requests served within a certain time (ms)
  50%    531
  66%    735
  75%    878
  80%    974
  90%   1234
  95%   1495
  98%   1809
  99%   2104
 100%   3805 (longest request)
```

2021-10-12, AX61-NVME with four discs and 8 partitions
```
Server Software:        nginx/1.18.0
Server Hostname:        135.181.182.4
Server Port:            80

Concurrency Level:      5
Time taken for tests:   264.412 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      274309399 bytes
HTML transferred:       274033261 bytes
Requests per second:    7.56 [#/sec] (mean)
Time per request:       661.029 [ms] (mean)
Time per request:       132.206 [ms] (mean, across all concurrent requests)
Transfer rate:          1013.12 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       13   27  16.1     25     348
Processing:    14  633 449.6    565    2996
Waiting:        0  590 425.7    520    2545
Total:         34  661 450.3    594    3014

Percentage of the requests served within a certain time (ms)
  50%    594
  66%    772
  75%    905
  80%   1000
  90%   1271
  95%   1510
  98%   1834
  99%   1997
 100%   3014 (longest request)
```

2021-10-12, AX61-NVME with four discs and 8 partitions
```
Server Software:        nginx/1.18.0
Server Hostname:        135.181.182.4
Server Port:            80

Concurrency Level:      5
Time taken for tests:   233.408 seconds
Complete requests:      2000
Failed requests:        0
Write errors:           0
Total transferred:      272488725 bytes
HTML transferred:       272213277 bytes
Requests per second:    8.57 [#/sec] (mean)
Time per request:       583.519 [ms] (mean)
Time per request:       116.704 [ms] (mean, across all concurrent requests)
Transfer rate:          1140.07 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:       12   25  10.1     24     187
Processing:    15  558 402.0    487    2727
Waiting:        0  512 377.0    440    2051
Total:         33  583 402.8    512    2757

Percentage of the requests served within a certain time (ms)
  50%    512
  66%    695
  75%    806
  80%    882
  90%   1114
  95%   1373
  98%   1621
  99%   1779
 100%   2757 (longest request)
```
