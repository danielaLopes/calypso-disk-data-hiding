1.  Added a second virtual disk in the VirtualBox interface, with 2 Gb, which corresponds to /dev/sdb

2. Copy files into the disk, which appears to be nearly completly formatted with a few exceptions

* text file as txt:
´´´
y$ sudo dd if=lusiadas.txt bs=4096 seek=0 of=/dev/sdb
´´´

* compressed file as zip:
´´´
$ sudo dd if=sdb/lusiadas.txt.zip seek=86 bs=4096 of=/dev/sdb
´´´

* pdf file:
´´´
$ sudo dd if=sdb/lusiadas.pdf seek=122 bs=4096 of=/dev/sdb
´´´

* Encrypted folder: (through Mac's encryption feature of 256 bit AES)
´´´
$ sudo dd if=sdb.dmg seek=525 bs=4096 of=/dev/sdb
´´´

* cat jpeg:
´´´
$ sudo dd if=sdb/Rainbow_Cat.jpeg seek=931 bs=4096 of=/dev/sdb
´´´