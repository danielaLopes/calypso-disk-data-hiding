#!/bin/sh

PHYSICAL_DISK_NAME="/dev/loop14"
TMP_FILE="/tmp/disk_data"
echo "abc" > "$PHYSICAL_DISK_NAME"
if ! insmod relay-disk.ko
then
	echo "insmod failed"
	exit 1
fi
if ! rmmod relay-disk
then
	echo "rmmod failed"
	exit 1
fi
sleep 1

echo -n "read from $PHYSICAL_DISK_NAME: "
dd if=$PHYSICAL_DISK_NAME of=$TMP_FILE count=3 bs=1 &> /dev/null
cat $TMP_FILE | hexdump -v -e '/1 "%02X "'; echo