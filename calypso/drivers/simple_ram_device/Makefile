ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default: module

module:
	make -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
 
else

	obj-m := simple_ram_device.o
	simple_ram_device-y := ../../lib/ram_io.o simple_ram_driver.o

endif