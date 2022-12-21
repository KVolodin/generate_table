#!/bin/bash

BUILD=$PWD/.build
[ -d $BUILD ] && rm -rf $BUILD
mkdir -p $BUILD && cd $BUILD

cmake -G "Unix Makefiles"                 \
      -D CMAKE_BUILD_TYPE:STRING=Debug    \
      ../
make

cp generate_table ../generate_table