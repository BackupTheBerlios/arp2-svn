# $Id$


### Definitions ###############################################################

INSTALL		= /gg/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA	= ${INSTALL} --mode=644

CPPFLAGS	= 
CFLAGS		= -m68020-60 -mnobitfield -fomit-frame-pointer \
                  -fstrict-aliasing -Os \
		  -W -Wall -Wstrict-prototypes -Wno-unused -g
LDFLAGS		= -noixemul -nostartfiles
LIBS		= 

OBJECTS		= main.o init.o controller.o devices.o pnp.o \
		  pnp_structs.o pnp_iterators.o
TARGET		= ISA-PnP


### Default rules #############################################################


%.o:		%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<


### Fake targets ##############################################################

.PHONY:		all clean distclean revup depend ddk

all:		$(TARGET)

clean:
		$(RM) -r $(OBJECTS) $(TARGET) version.h ddk

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

ddk:
	$(INSTALL) --directory $@/include/lvo
	$(INSTALL) --directory $@/storm/pragma $@/storm/proto
	$(INSTALL) --directory $@/sas/pragmas $@/sas/proto
	$(INSTALL) --directory $@/gcc/inline $@/gcc/ppcinline $@/gcc/proto
	cp -a include $@
	$(RM) -r `/bin/find $@ -name CVS`
	fd2pragma $@/include/fd/isapnp_lib.fd 22 TO $@/include/lvo/isapnp_lib.i
	fd2pragma $@/include/fd/isapnp_lib.fd 5  TO $@/storm/pragma/isapnp_lib.h COMMENT
	fd2pragma $@/include/fd/isapnp_lib.fd 30 TO $@/storm/proto/isapnp.h
	fd2pragma $@/include/fd/isapnp_lib.fd 3  TO $@/sas/pragmas/isapnp_pragmas.h COMMENT
	fd2pragma $@/include/fd/isapnp_lib.fd 33 TO $@/sas/proto/isapnp.h
	fd2inline --new $@/include/fd/isapnp_lib.fd $@/include/clib/isapnp_protos.h -o $@/gcc/inline/isapnp.h
	fd2inline --new --morphos $@/include/fd/isapnp_lib.fd $@/include/clib/isapnp_protos.h -o $@/gcc/ppcinline/isapnp.h
	fd2inline --proto --morphos $@/include/fd/isapnp_lib.fd -o $@/gcc/proto/isapnp.h

version.h:	version.rev version.date
	@echo Creating $@
	@ver=`cat version.ver` \
	 rev=`cat version.rev` \
	 date=`cat version.date` && \
	 echo "#define VERSION $$ver" > $@ && \
	 echo "#define REVISION $$rev" >> $@ && \
	 echo "#define VERS \"$$ver.$$rev ($$date)\"" >> $@


### Real targets ##############################################################

$(TARGET):	$(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@


### Dependencies ##############################################################

depend Makefile.dep:	version.h
	$(CC) $(CPPFLAGS) -MM $(wildcard *.c) > Makefile.dep

include Makefile.dep
