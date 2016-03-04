#!/bin/bash

pid=0

make
make install
echo
echo "echo ${pid} > /proc/get_proc_info"
echo ${pid} > /proc/get_proc_info
echo
make uninstall
