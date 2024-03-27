#!/bin/bash

# Execute with:
# $ chmod +x compile.sh
# $ ./compile.sh
 
TABLE=$(sudo grep sys_call_table /boot/System.map-$(uname -r) |awk '{print $1}')
sed -i s/TABLE/$TABLE/g hijack.c
 
make