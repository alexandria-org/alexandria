#!/bin/bash

cd `dirname $0`
cd ..

export CC=/usr/bin/gcc-10
export CXX=/usr/bin/g++-10

base_path=`pwd`
cd $base_path

mkdir -p deps
cd deps
#rm -rf ./*

curl https://zlib.net/zlib-1.2.11.tar.gz > zlib-1.2.11.tar.gz
gunzip zlib-1.2.11.tar.gz
tar -xvf zlib-1.2.11.tar

cd $base_path
cd deps

git clone https://github.com/awslabs/aws-lambda-cpp.git

cd $base_path
cd deps

git clone https://github.com/aws/aws-sdk-cpp.git
cd aws-sdk-cpp
git checkout tags/1.9.47
git submodule update --init --recursive

