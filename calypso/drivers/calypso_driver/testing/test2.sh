#! /bin/sh


FIRST_BLOCK=12157

BLOCK=$FIRST_BLOCK
for i in {0..100}
do
    ((BLOCK++))
   sudo dd if=testing/middle_block.txt seek=${BLOCK} count=1 bs=4096 of=/dev/sda8
   #echo "sudo dd if=testing/middle_block.txt seek=${BLOCK}  count=1 bs=4096 of=/dev/sda8"
done

BLOCK=$FIRST_BLOCK
for i in {0..100}
do
    ((BLOCK++))
   sudo dd if=/dev/sda8 skip=${BLOCK} count=1 bs=4096
   #echo "sudo dd if=/dev/sda8 skip=${BLOCK} count=1 bs=4096"
done

BLOCK=$FIRST_BLOCK
for i in {100..200}
do
    ((BLOCK++))
   sudo dd if=/dev/sda8 skip=${BLOCK} count=1 bs=4096
   #echo "sudo dd if=/dev/sda8 skip=${BLOCK} count=1 bs=4096"
done

echo "Issued 100 write requests and 200 read requests"