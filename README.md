# Alexandria.org

## Documentation
1. [Index file format (.fti)](/documentation/index_file_format.md)
2. [Search Result Ranking](/documentation/search_result_ranking.md)
3. [API Response format](/documentation/api_response_format.md)

## Notes

This is a pretty nice open source font: https://rsms.me/inter/
Found it here: https://news.ycombinator.com/item?id=28009042

## How to build and deploy
1. Configure the system
```
./configure
```

2. Build with cmake
```
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=./deps/out
or
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=./deps/out

make
make run_tests
```

3. Deploy to lambda/ec2/alexandria
```
./scripts/deploy.sh lambda
./scripts/deploy.sh ec2
./scripts/deploy.sh alexandria
```

## Launch ec2
```
./scripts/launch_ec2.sh
```
