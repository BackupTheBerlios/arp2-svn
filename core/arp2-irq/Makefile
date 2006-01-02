
obj-m		+= arp2-irq.o divdi3.o
arp2-irq-objs	:= irq.o pcode.o
divdi3-objs	:= divdi3_exp.o divdi3_impl.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:	clean-test
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


test:	pcode-test.bin.c

clean-test:
	$(RM) pcode-test.bin.c pcode-test.bin pcode-test

pcode-test.bin.c:pcode-test.bin
	bin2c pcode_test < $< > $@

pcode-test.bin:	pcode-test
	mmix-mmixware-objcopy -O binary $^ $@

pcode-test:	pcode-test.c pcode-traps.h
	mmix-mmixware-gcc -W -Wall -O2 -nostdlib $< -o $@
