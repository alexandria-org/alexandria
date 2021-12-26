# Alexandria.org

## Documentation
1. [Index file format (.fti)](/documentation/index_file_format.md)
2. [Search Result Ranking](/documentation/search_result_ranking.md)
3. [API Response format](/documentation/api_response_format.md)
4. [Caching](/documentation/caching.md)
5. [Installing nodes](/documentation/installing_nodes.md)

## Build with docker
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
4. Initialize docker
```
/alexandria/scripts/init-docker.sh
```
5. Download and build dependencies.
```
/alexandria/scripts/download-deps.sh
/alexandria/scripts/build-deps.sh
```
6. Configure with cmake and build tests.
```
mkdir /alexandria/build
cd /alexandria/build

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=./deps/out
or
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=./deps/out

make -j4 run_tests
```

## How to build manually
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

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=./deps/out
or
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=./deps/out

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

## Coding rules
1. Never put "using namespace..." in header files.
2. Namspaces and Classes written by us should be CamelCase
3. Everything else should be lower_case
4. All files within a sub directory must contain namespace that is the same as the directory. For example src/file/TsvFile.h must declare everything within the namespace File.
