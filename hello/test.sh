#!/bin/bash

name="jianxiang fan"

make
echo
make install name="${name}"
make uninstall
echo "dmesg | tail -n2"
dmesg | tail -n2