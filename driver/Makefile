# Out-of-tree kernel module build. Delegates to the kernel's Kbuild system,
# which compiles kshellnote.c against the headers of the running kernel.
obj-m := kshellnote.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean load unload install-udev

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: all
	sudo insmod kshellnote.ko

unload:
	sudo rmmod kshellnote

# Let non-root users open /dev/kshellnote (udev applies MODE=0666 on create).
install-udev:
	sudo install -m 644 99-kshellnote.rules /etc/udev/rules.d/99-kshellnote.rules
	sudo udevadm control --reload-rules
