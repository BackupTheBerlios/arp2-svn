
INSTALL		= /gg/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA	= ${INSTALL} --mode=644

CPPFLAGS	= 
CFLAGS		= -m68020-60 -mnobitfield -fomit-frame-pointer \
                  -fstrict-aliasing -Os \
		  -W -Wall -Wstrict-prototypes -g
LDFLAGS		= -noixemul -lamiga

TARGET		= ISA-PnP


.PHONY:		all clean distclean revup depend 

all:		$(TARGET)

clean:
		$(RM) *.o $(TARGET) version.h

distclean:	clean
	$(RM) Makefile.dep

install:	all
	$(INSTALL_PROGRAM) --strip $(TARGET) /SYS/Expansion
	$(INSTALL_DATA) $(TARGET).info /SYS/Expansion

revup:
	@rev=`cat version.rev` && echo `expr $$rev + 1` > version.rev
	@date +%d.%m.%y > version.date
	@echo -n "New revision: "
	@cat version.rev

version.h:	version.rev version.date
	@echo Creating $@
	@ver=`cat version.ver` \
	 rev=`cat version.rev` \
	 date=`cat version.date` && \
	 echo "#define VERSION $$ver" > $@ && \
	 echo "#define REVISION $$rev" >> $@ && \
	 echo "#define VERS \"$$ver.$$rev ($$date)\"" >> $@

%.o:		%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<

init.o:		version.h

$(TARGET):	main.o init.o controller.o
	$(CC) $(LDFLAGS) $^ -o $@

depend Makefile.dep:	version.h
	$(CC) $(CPPFLAGS) -MM $(wildcard *.c) > Makefile.dep

include Makefile.dep
