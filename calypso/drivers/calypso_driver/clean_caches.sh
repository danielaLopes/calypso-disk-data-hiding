#! /bin/sh

# $ bash clean_caches.sh 

DISK=/dev/sda8


echo "----- Cleaning caches -----"

# Clear buffer caches so it will read the mos updated value from disk
sync
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
sudo blockdev --flushbufs $DISK
sudo hdparm -F $DISK