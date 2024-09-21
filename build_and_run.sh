#!/bin/bash
rm -rf build
rm -rf tests/*.so
cp build/lib.macosx-10.13-x86_64-cpython-312/cmake_example.cpython-312-darwin.so tests/

python setup.py build

# cd tests
# export DYLD_LIBRARY_PATH=/usr/local/Cellar/ffmpeg/7.0.2_1/lib:$DYLD_LIBRARY_PATH
# python test.py
# cd -
