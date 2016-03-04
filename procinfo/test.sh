#!/bin/bash

pid=0

make
make install
echo "gcc main.c -o test"
gcc main.c -o test
echo
echo "./test ${pid}"
./test ${pid}
echo
make uninstall