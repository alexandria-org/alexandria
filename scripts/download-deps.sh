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

curl https://zlib.net/zlib-1.2.12.tar.gz > zlib-1.2.12.tar.gz
gunzip -f zlib-1.2.12.tar.gz
tar -xvf zlib-1.2.12.tar

git clone https://github.com/abseil/abseil-cpp.git
git clone https://github.com/RoaringBitmap/CRoaring.git
wget https://raw.githubusercontent.com/google/robotstxt/master/robots.cc
wget https://raw.githubusercontent.com/google/robotstxt/master/robots.h

