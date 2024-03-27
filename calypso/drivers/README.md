# Setup

## Virtual machine
```
$ sudo mount -t vboxsf -o uid=1000,gid=1000 thesis /home/daniela/vboxshare

$ cd /home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver

$ make clean

$ make 

$ sudo insmod calypso_driver.ko blocks=1000
```


## Create disk images
* one with block size 1k:
```
$ sudo dd if=/dev/zero of=relay_disk_1k.img bs=512 count=1000
$ sudo mkfs.ext4 -b 1024 relay_disk_1k.img
```

* another one with block size 4k:
```
$ sudo dd if=/dev/zero of=relay_disk.img bs=512 count=1000
$ sudo mkfs.ext4 relay_disk.img
```

* check block sizes of filesystems:
```
$ tune2fs relay_disk2.img -l
```

* **NOTE:** this does not matter much to us. What matters is the block size of the calypso block device, which is the size that determines the amount of sectors transfered on each I/O operation.


## Check block size of Calypso block device
```
$ sudo blockdev --getbsz /dev/calypso0
> 4096
```

* **NOTE:** this is the reason each read or write causes the struct request to have 8 sectors, since it needs to transfer a whole block in each I/O operation. Each sector is 512 bytes, each block is 4096 bytes, so we need to transfer 4096 / 512 = 8 sectors in each I/O operation



## Associate loop device with disk image
```
sudo losetup -fP relay_disk.img
```

## List loop devices and associated images
```
$ losetup -a
```

## Mount loop device 
```
$ sudo mount -o loop /dev/loop14 ~/relay_disk2
```


# Usage
## Insert calypso driver module
* To simulate a RAM device:
```
$ sudo insmod calypso_driver.ko io_mode=0
```

* To write changes to a file:
```
$ sudo insmod calypso_driver.ko io_mode=1
```

* To write changes to disk:
```
$ sudo insmod calypso_driver.ko io_mode=2
```


## Remove calypso driver module
```
sudo rmmod calypso_driver
```


## Writing and reading specific sectors of device
```
$ sudo su
$ echo "abc" > /dev/calypso0
$ echo "def" > /dev/calypso0
$ echo "ghi" > /dev/calypso0
$ exit
```

Read image data from a particular sector (900):
```
$ img_cat -s 900 relay_disk.img
> abc
> def
> ghi
```

```
$ img_cat -s 917 relay_disk.img
> ghi
```

```
$ img_cat -s 909 relay_disk.img
> def
> ghi
```

* **NOTE:** this happens because the sectors seem to be incremented by 8 with each request, still don't quite understand why

## Big write requests
* This emits an error, blocks the system and cannot be processed:
```
$ python3 -c "print('A'*512*8)" > a.txt
$ sudo su
$ dd if=a.txt of=/dev/calypso0
$ exit
```

## Tests 
* Write to first block:
```
$ sudo dd if=first_block.txt seek=0 bs=512 of=/dev/calypso0


$ sudo xxd /dev/calypso0 | grep "first"
or 
$ sudo xxd calypso.dat | grep "first"

> 00000000: 6669 7273 7420 626c 6f63 6b0a 0000 0000  first block.....
```

* Write to second block:
```
$ sudo dd if=second_block.txt seek=1 bs=512 of=/dev/calypso0


$ sudo xxd /dev/calypso0 | grep "second"
or 
$ sudo xxd calypso.dat | grep "second"

> 00000200: 7365 636f 6e64 2062 6c6f 636b 0a00 0000  second block....
```

* Write to third block:
```
$ sudo dd if=third_block.txt seek=2 bs=512 of=/dev/calypso0


$ sudo xxd /dev/calypso0 | grep "third"
or 
$ sudo xxd calypso.dat | grep "third"

> 00000400: 7468 6972 6420 626c 6f63 6b0a 0000 0000  third block.....
```

* Write to last block:
    * /dev/calypso has 1000 blocks, so last block is 999
```
$ sudo dd if=last_block.txt seek=999 bs=512 of=/dev/calypso0


$ sudo xxd /dev/calypso0 | grep last
or 
$ sudo xxd calypso.dat | grep "last"

> 0007ce00: 6c61 7374 2062 6c6f 636b 0a00 0000 0000  last block......
```

## Cleanup calypso.dat file:
```
$ dd if=/dev/null bs=512 count=1000 of=calypso.dat 
```


# Debug
## dmesg
* last boot:
```
journalctl -o short-precise -k -b -1
```


# Cleanup
```
sudo losetup --detach /dev/loop14
```


```
$ sudo hdparm --fibmap /home/daniela/hello.txt 

>/home/daniela/hello.txt:
> filesystem blocksize 4096, begins at LBA 1052672; assuming 512 byte sectors.
> byte_offset  begin_LBA    end_LBA    sectors
>           0   26974752   26974759          8
```

```
$ filefrag -v /home/daniela/hello.txt -b4096

>Filesystem type is: ef53
>File size of /home/daniela/hello.txt is 12 (1 block of 4096 bytes)
> ext:     logical_offset:        physical_offset: length:   expected: flags:
>   0:        0..       0:    3240260..   3240260:      1:             last,eof
>/home/daniela/hello.txt: 1 extent found
```

```
Inode: 1046644   Type: regular    Mode:  0664   Flags: 0x80000
Generation: 3691815496    Version: 0x00000000:00000001
User:  1000   Group:  1000   Project:     0   Size: 12
File ACL: 0
Links: 1   Blockcount: 8
Fragment:  Address: 0    Number: 0    Size: 0
 ctime: 0x60414457:7f289294 -- Thu Mar  4 20:34:31 2021
 atime: 0x60414436:abe531c4 -- Thu Mar  4 20:33:58 2021
 mtime: 0x60414457:7f289294 -- Thu Mar  4 20:34:31 2021
crtime: 0x60414436:abe531c4 -- Thu Mar  4 20:33:58 2021
Size of extra inode fields: 32
Inode checksum: 0x046df79b
EXTENTS:
(0):3240260


3240260 * 8 = 25922080
```

# Filesystem
* check superblock info on partition /dev/sda8:
```
$ sudo dumpe2fs -h /dev/sda8

>dumpe2fs 1.45.5 (07-Jan-2020)
Filesystem volume name:   <none>
Last mounted on:          /media/calypso
Filesystem UUID:          3ccad46c-b546-4d87-9a4d-fe6bde8de35d
Filesystem magic number:  0xEF53
Filesystem revision #:    1 (dynamic)
Filesystem features:      has_journal ext_attr resize_inode dir_index filetype needs_recovery extent 64bit flex_bg sparse_super large_file huge_file dir_nlink extra_isize metadata_csum
Filesystem flags:         signed_directory_hash 
Default mount options:    user_xattr acl
Filesystem state:         clean
Errors behavior:          Continue
Filesystem OS type:       Linux
Inode count:              418608
Block count:              1671168
Reserved block count:     83558
Free blocks:              1179415
Free inodes:              406956
First block:              0
Block size:               4096
Fragment size:            4096
Group descriptor size:    64
Reserved GDT blocks:      815
Blocks per group:         32768
Fragments per group:      32768
Inodes per group:         8208
Inode blocks per group:   513
Flex block group size:    16
Filesystem created:       Fri Mar 12 16:28:00 2021
Last mount time:          Sun Mar 14 22:37:52 2021
Last write time:          Sun Mar 14 22:37:52 2021
Mount count:              28
Maximum mount count:      -1
Last checked:             Fri Mar 12 16:28:00 2021
Check interval:           0 (<none>)
Lifetime writes:          3709 MB
Reserved blocks uid:      0 (user root)
Reserved blocks gid:      0 (group root)
First inode:              11
Inode size:	          256
Required extra isize:     32
Desired extra isize:      32
Journal inode:            8
Default directory hash:   half_md4
Directory Hash Seed:      3b67b401-a381-46ec-b43d-2107b6446df2
Journal backup:           inode blocks
Checksum type:            crc32c
Checksum:                 0x76f527fa
Journal features:         journal_incompat_revoke journal_64bit journal_checksum_v3
Journal size:             64M
Journal length:           16384
Journal sequence:         0x00009915
Journal start:            0
Journal checksum type:    crc32c
Journal checksum:         0x8f015a1e
```

# Building out-of-tree modules
* kernel header files are place on /usr/src/$(uname -r)/include/linux

# Writing test to free blocks in /dev/sda8
## Test 1
* This request has to present atleast two write requests intercepted, the rest can be cached
```
$ sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096
```

```
$ sudo dd if=testing/middle_block.txt seek=12157 count=1 bs=4096 of=/dev/sda8
```

```
$ sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096
```

```
$ sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096
```

```
$ sudo dd if=testing/first_block.txt seek=12157 count=1 bs=4096 of=/dev/sda8
```

```
$ sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096
```

```
$ sudo dd if=/dev/sda8 skip=12157 count=1 bs=4096
```

## Test 2