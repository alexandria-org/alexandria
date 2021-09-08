#!/bin/bash

cd `dirname $0`
cd ..

export CC=/usr/bin/gcc-10
export CXX=/usr/bin/g++-10

base_path=`pwd`
cd $base_path

cd deps

cd zlib-1.2.11
./configure
make -j4
make install

cd $base_path
cd deps

cd aws-lambda-cpp
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=../../out -DCPP_STANDARD=17
make -j4 && make install

cd $base_path
cd deps

cd aws-sdk-cpp
mkdir build
cd build
cmake .. -DBUILD_ONLY="s3;transfer;lambda" -DBUILD_SHARED_LIBS=OFF -DENABLE_UNITY_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../out -DENABLE_TESTING=OFF -DBUILD_DEPS=ON -DCPP_STANDARD=17 -DAWS_LIBCRYPTO_LOG_RESOLVE=0
make -j4
make install

