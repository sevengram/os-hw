#!/bin/bash

make
make install
echo "gcc main.c -o test"
gcc main.c -o test
echo
echo "run test..."
./test 0
echo
make uninstall