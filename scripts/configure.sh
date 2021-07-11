#!/bin/bash

cd `dirname $0`
cd ..

export CC=/usr/bin/gcc-10
export CXX=/usr/bin/g++-10

sudo apt-get install zip make cmake gcc-10 g++-10 libcurl4-openssl-dev libssl-dev libcrypto++-dev libboost-iostreams-dev libboost-filesystem-dev libboost-system-dev libfcgi-dev spawn-fcgi nginx

rel_path=`pwd`
base_path=`realpath $rel_path`
cd $base_path

mkdir -p deps
cd deps
rm -rf ./*

wget https://zlib.net/zlib-1.2.11.tar.gz
gunzip zlib-1.2.11.tar.gz
tar -xvf zlib-1.2.11.tar
cd zlib-1.2.11
./configure
make -j8
make install
cd ..

git clone https://github.com/awslabs/aws-lambda-cpp.git
cd aws-lambda-cpp
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=../../out -DCPP_STANDARD=17
make -j8 && make install

cd $base_path
cd deps

git clone https://github.com/aws/aws-sdk-cpp.git
cd aws-sdk-cpp
git checkout tags/1.9.47
git submodule update --init --recursive
mkdir build
cd build
cmake .. -DBUILD_ONLY="s3;transfer" -DBUILD_SHARED_LIBS=OFF -DENABLE_UNITY_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../out -DENABLE_TESTING=OFF -DBUILD_DEPS=ON -DCPP_STANDARD=17 -DAWS_LIBCRYPTO_LOG_RESOLVE=0
make -j8
make install

# Install nginx config.


