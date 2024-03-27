MODULE_NAME := blk_mq_driver2
obj-m := $(MODULE_NAME).o

#OBJ_LIST := blk_mq_driver2.o
#$(MODULE_NAME)-y += $(OBJ_LIST)

ccflags-y := -O2

KERNELDIR := /lib/modules/$(shell uname -r)/build

all: blk_mq_driver2

blk_mq_driver2:
	make -C $(KERNELDIR) M=$(PWD) modules