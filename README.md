# Alexandria.org

1. [Coding Rules](/documentation/coding_rules.md)
2. [Full text indexes](/documentation/full_text_indexes.md)
3. [Hash table](/documentation/hash_table.md)

## Build instructions with docker
1. Build docker image
```
docker build . -t alexandria
```

2. Run container
```
docker container run --name alexandria -v $PWD:/alexandria -it -d alexandria
```
3. Attach to container.
```
docker exec -it alexandria /bin/bash
```
4. Navigate to directory
```
cd /alexandria
```
5. Initialize docker
```
scripts/init-docker.sh
```
5. Download and build dependencies.
```
scripts/download-deps.sh
scripts/build-deps.sh
```
6. Configure with cmake and build tests.
```
mkdir build; cd build; 

## Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
## or build with compiler optimization
cmake .. -DCMAKE_BUILD_TYPE=Release

## build test suite
make -j4 run_tests

## run test suite
./run_tests
```

## How to build manually (not recommended)
1. Configure the system (Tested on Ubuntu 20.04)
```
# Will alter your system and install dependencies with apt.
./scripts/install-deps.sh

# Will download and build zlib, aws-lambda-cpp and aws-sdk-cpp will only alter the local directory.
./scripts/build-deps.sh
```

2. Build with cmake
```
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Debug
or
cmake .. -DCMAKE_BUILD_TYPE=Release

make -j24
```

3. Download test data to local server.
To run the test suite you need to install nginx and pre-download all the data: [Configure local nginx test data server](/documentation/configure_local_nginx.md)

4. Create output directories. Note, this will create a bunch of directories in the /mnt so make sure you don't have anything there.
```
./scripts/prepare-output-dirs.sh
```

5. Run the test suite
```
cd build
make run_tests -j24
./run_tests
```

## Notes
On nodes with spinning disks we should turn off energy saving:
```
hdparm -B 255 /dev/sda
```

## Debugging notes
### Debugging scraper with gdb:
By default, gdb captures SIGPIPE of a process and pauses it. However, some program ignores SIGPIPE. So, the default behavour of gdb is not desired when debugging those program. To avoid gdb stopping in SIGPIPE, use the folloing command in gdb:
```handle SIGPIPE nostop noprint pass```
