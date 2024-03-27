#! /bin/sh

sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096

sudo dd if=testing/middle_block.txt seek=12157 count=1 bs=4096 of=/dev/sda8

sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096

sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096

sudo dd if=testing/first_block.txt seek=12157 count=1 bs=4096 of=/dev/sda8

sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096

sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096

echo "Issued 2 write requests and 5 read requests"