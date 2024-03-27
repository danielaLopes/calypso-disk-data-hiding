#obj-m += blkdev.o
obj-m += sbull.o
obj-m += sbull_ram.o
obj-m += sbull_hello.o
obj-m += sbull_parts.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean