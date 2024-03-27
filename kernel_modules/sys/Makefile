obj-m += sysfs1.o
obj-m += sysfs2.o
obj-m += sysfs3.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean