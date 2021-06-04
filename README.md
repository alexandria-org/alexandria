# cc-parser

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
