# Makefile for ARP ROM image (m68k)

CC		= m68k-amigaos-gcc
OBJCOPY		= m68k-amigaos-objcopy
OBJDUMP		= m68k-amigaos-objdump

CFLAGS		= -W -Wall -g -Os -fomit-frame-pointer -noixemul #-m68020
CPPFLAGS	= -I../arp2-syscall -I../arp2-syscall/test

OBJECTS		= autoconf.o resident.o
TARGET		= arp2.rom
TARGET_BIN	= arp2.bin

all:	$(TARGET) #$(TARGET_BIN)

clean:
	$(RM) $(OBJECTS) $(TARGET) $(TARGET_BIN) Makefile.dep

$(TARGET):	$(OBJECTS)
	$(CC) -nostartfiles -noixemul $(OBJECTS) $(OUTPUT_OPTION)

$(TARGET_BIN):	$(TARGET)
	$(OBJCOPY) --output-target=binary $< $@ 

Makefile.dep:	*.c
	$(CC) $(CPPFLAGS) -MM *.c > Makefile.dep

include Makefile.dep

