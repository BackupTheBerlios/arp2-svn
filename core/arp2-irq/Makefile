
obj-m		+= arp2-irq.o divdi3.o
arp2-irq-objs	:= irq.o pcode.o
divdi3-objs	:= divdi3_exp.o divdi3_impl.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
