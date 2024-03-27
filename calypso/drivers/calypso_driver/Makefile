DEBUG_CFLAGS += -g -DDEBUG

ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default: module

module:
	make -C $(KERNEL_SOURCE) M=$(PWD) modules

# TODO
#test: 

#debug:
	#make -C $(KERNEL_SOURCE) M=$(PWD) modules
	#EXTRA_CFLAGS="$(DEBUG_CFLAGS)"

clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean

reset:
	rm calypso_mappings.dat 
	rm calypso_bitmap.dat
 
else

	obj-m := calypso_driver.o
	calypso_driver-y := ../../lib/ram_io.o ../../lib/file_io.o ../../lib/disk_io.o \
						../../lib/physical_device.o ../../lib/virtual_device.o \
						../../lib/fs_utils.o ../../lib/ext4_fs_utils.o \
						../../lib/persistence.o ../../lib/data_hiding.o \
						../../lib/requests.o ../../lib/mtwister.o \
						../../lib/hkdf.o ../../lib/block_encryption.o \
						../../lib/block_hashing.o ../../lib/disk_entropy.o \
						driver.o

endif