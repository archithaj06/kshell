# Out-of-tree kernel module build. Delegates to the kernel's Kbuild system,
# which compiles kshellnote.c against the headers of the running kernel.
obj-m := kshellnote.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean load unload

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: all
	sudo insmod kshellnote.ko

unload:
	sudo rmmod kshellnote
