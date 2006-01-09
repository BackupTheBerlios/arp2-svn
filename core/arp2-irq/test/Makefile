
CPPFLAGS	= -Wall -I..
CFLAGS		= -g -O2

all:	pcode-main

clean:
	$(RM) pcode-test.bin.c pcode-test.bin pcode-test pcode-main

pcode-test.bin.c:pcode-test.bin
	bin2c pcode_test < $< > $@

pcode-test.bin:	pcode-test
	mmix-mmixware-objcopy -O binary $^ $@

pcode-test:	pcode-test.c pcode-traps.h
	mmix-mmixware-gcc -W -Wall -O2 -nostdlib $< $(OUTPUT_OPTION)

pcode-main:	pcode-main.c pcode-test.bin.c ../pcode.c
