#!/bin/bash

# Check code with cppcheck
# Read more about ccpcheck here https://cppcheck.sourceforge.io/manual.pdf

cd `dirname $0`
cd ../build

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

cppcheck --suppress=*:json.hpp --suppress=useStlAlgorithm -i deps -q --enable=all --project=compile_commands.json 2>&1
