
srcdir		= .

CC		= m68k-amigaos-gcc
CPPFLAGS	= #-DDEBUG
#CPPFLAGS	= -DTEST
CFLAGS		= -O2 -fomit-frame-pointer
LDFLAGS		= -noixemul -nostartfiles -s
#LDFLAGS	= -noixemul

FD2INLINE	= fd2inline
FD2INLINE_TARGET= --target=m68k-unknown-amigaos

all:		vt82c

clean:		
	$(RM) vt82c *.o version.h camdstubs.c camdstubs.h *~

revup:
	@rev=`cat $(srcdir)/version.rev` && echo `expr $$rev + 1` > $(srcdir)/version.rev
	@date +%d.%m.%y > $(srcdir)/version.date
	@echo -n "New revision: "
	@cat $(srcdir)/version.rev



vt82c:		vt82c.o camdstubs.o
	$(CC) $(LDFLAGS) $^ -o $@

vt82c.o:	vt82c.c camdstubs.h version.h

camdstubs.c:	camd_drv.fd camd_drv_protos.h
	$(FD2INLINE) --mode=gatestubs --gateprefix=gw --libprefix=_ \
	             $(FD2INLINE_TARGET) \
	             $^ -o $@

camdstubs.h:	camd_drv.fd camd_drv_protos.h
	$(FD2INLINE) --mode=gateproto --gateprefix=gw --libprefix=_ \
	             $(FD2INLINE_TARGET) \
	             $^ -o $@

camdstubs.o:	camdstubs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -D__CLIB_TYPES__ -c $^ $(OUTPUT_OPTION)

version.h:	version.ver version.rev version.date
	@echo Creating $@
	@ver=`cat $(srcdir)/version.ver` \
	 rev=`cat $(srcdir)/version.rev` \
	 date=`cat $(srcdir)/version.date` && \
	 echo "#define VERSION $$ver" > $@ && \
	 echo "#define REVISION $$rev" >> $@ && \
	 echo "#define VERS \"$$ver.$$rev ($$date)\"" >> $@
