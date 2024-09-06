#!/bin/sh

# Unix Makefiles version with Clang and LLVM
rm -rf build
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -D_CMAKE_TOOLCHAIN_PREFIX=llvm-
