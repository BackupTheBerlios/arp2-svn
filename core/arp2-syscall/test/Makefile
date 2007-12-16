
CC		= m68k-amigaos-gcc
CPPFLAGS	= -I. -I..
CFLAGS		= -W -Wall -O2 -g 
LDFLAGS		= -noixemul

SFDC		= sfdc

SFD_FILE	= ../arp2sys_lib.sfd

HEADERS		= clib/arp2_syscall_protos.h inline/arp2_syscall.h proto/arp2_syscall.h
TARGETS		= arp2sys-test

all:	$(HEADERS) $(TARGETS)

clib/arp2_syscall_protos.h:	$(SFD_FILE)
	$(SFDC) --mode=clib $< -o $@

inline/arp2_syscall.h:	$(SFD_FILE)
	$(SFDC) --mode=macros $< -o $@

proto/arp2_syscall.h:	$(SFD_FILE)
	$(SFDC) --mode=proto $< -o $@

