obj-m += chardev.o
obj-m += chardev_baseline.o
obj-m += chardev_impl.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean