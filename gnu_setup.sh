#!/bin/sh

# Unix Makefiles version with default compiler which is GNU
rm -rf build
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
