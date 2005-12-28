
obj-m		+= arp2-irq.o divdi3.o
arp2-irq-objs	:= irq.o pcode.o
divdi3-objs	:= divdi3_exp.o divdi3_impl.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


# obj-m    := arp2-irq.o

# INSTDIR := /lib/modules/$(shell uname -r)/kernel/drivers/char
# KDIR    := /lib/modules/$(shell uname -r)/build
# PWD    := $(shell pwd)

# default:
# 	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

# install:
# 	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install

# clean:
# 	$(RM) arp2-irq.ko arp2-irq.o arp2-irq.mod.c arp2-irq.mod.o
