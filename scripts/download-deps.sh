#!/bin/bash

cd `dirname $0`
cd ..

export CC=/usr/bin/gcc-10
export CXX=/usr/bin/g++-10

base_path=`pwd`
cd $base_path

mkdir -p deps
cd deps

curl -L https://github.com/nlohmann/json/releases/latest/download/json.hpp > json.hpp

curl https://zlib.net/zlib-1.2.11.tar.gz > zlib-1.2.11.tar.gz
gunzip zlib-1.2.11.tar.gz
tar -xvf zlib-1.2.11.tar

git clone --recurse-submodules https://github.com/google/leveldb.git

