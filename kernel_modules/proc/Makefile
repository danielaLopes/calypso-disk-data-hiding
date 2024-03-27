obj-m += procfs_hello.o
obj-m += procfs_interface.o
obj-m += procfs_inode.o
#obj-m += procfs_inode2.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean