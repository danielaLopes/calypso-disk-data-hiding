ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default: module

module:
	make -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
 
else

	obj-m := loop_snoop_driver.o
	loop_snoop_driver-y := ../../lib/ram_io.o ../../lib/file_io.o ../../lib/disk_io.o \
					../../lib/physical_device.o ../../lib/virtual_device.o \
					../../lib/fs_utils.o ../../lib/ext4_fs_utils.o \
					driver.o

endif