1. Load the driver dor.ko using insmod. This would create the block device files representing the disk on 512 KiB of RAM, with three primary and three logical partitions.

2. Check out the automatically created block device files (/dev/rb*). /dev/rb is the entire disk, which is 512 KiB in size. rb1, rb2 and rb3 are the primary partitions, with rb2 being the extended partition and containing three logical partitions rb5, rb6 and rb7.

3. Read the entire disk (/dev/rb) using the disk dump utility dd.
```
sudo dd if=/dev/rb
```

4. Zero out the first sector of the disk’s first partition (/dev/rb1), again using dd.
```
sudo dd if=/dev/zero of=/dev/rb1 count=1
```

5. Write some text into the disk’s first partition (/dev/rb1) using cat.
```
sudo su
cat > /dev/rb1
oldsfknvjsnbvjkgfn v ^C
exit
```

6. Display the initial contents of the first partition (/dev/rb1) using the xxd utility. See Figure 2 for xxd output.
```
sudo xxd /dev/rb1
```

7. Display the partition information for the disk using fdisk. See Figure 3 for fdisk output.
Quick-format the third primary partition (/dev/rb3) as a vfat filesystem (like your pen drive), using mkfs.vfat (Figure 3).
```
mkfs.vfat /dev/rb3
```

8. Mount the newly formatted partition using mount, say at /mnt (Figure 3).
The disk usage utility df would now show this partition mounted at /mnt (Figure 3). You may go ahead and store files there, but remember that this is a disk on RAM, and so is non-persistent.

9. Unload the driver using rmmod dor after unmounting the partition using umount /mnt. All data on the disk will be lost.
