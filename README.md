# Alexandria.org

## Documentation
1. [Index file format (.fti)](/documentation/index_file_format.md)
2. [Search Result Ranking](/documentation/search_result_ranking.md)
3. [API Response format](/documentation/api_response_format.md)

## How to build
1. Configure the system
Tested on Ubuntu 20.04.
```
# Will alter your system and install dependencies with apt.
./scripts/install-deps.sh

# Will download and build zlib, aws-lambda-cpp and aws-sdk-cpp will only alter the local directory.
./scripts/configure.sh
```

2. Build with cmake
```
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=./deps/out
or
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=./deps/out

make
```

3. Download test data to local server.
To run the test suite without having to download all the test data while running the test suite you need to install nginx and pre-download all the data.
[Configure local nginx test data server](/documentation/configure_local_nginx.md)

4. Run the test suite
```
cd build
./run_tests
```

