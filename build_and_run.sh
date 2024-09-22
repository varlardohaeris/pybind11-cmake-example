#!/bin/bash
rm -rf build
rm -rf tests/*.so

python3 setup.py build

cp build/lib.linux-x86_64-cpython-312/cmake_example.cpython-312-x86_64-linux-gnu.so tests/
BATCH_SIZE=12
MP4_PATH="/root/repo/cmake_example/test.mp4"
cd tests
# gdb --args python3 test.py $BATCH_SIZE $MP4_PATH
python3 test.py $BATCH_SIZE $MP4_PATH
cd -
