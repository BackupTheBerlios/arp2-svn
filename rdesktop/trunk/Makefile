#
# rdesktop: A Remote Desktop Protocol client
# Makefile
# Copyright (C) Matthew Chapman 1999-2001
#

# Configuration defaults

CC       = gcc
CFLAGS   = -O2 -Wall
INCLUDES = -I$(shell pwd)
LDLIBS   =

PREFIX   = /usr/local
EPREFIX  = $(PREFIX)
BINDIR   = $(EPREFIX)/bin
MANDIR   = $(PREFIX)/man

RDPOBJ   = rdesktop.o tcp.o iso.o mcs.o secure.o licence.o rdp.o orders.o bitmap.o cache.o decompress.o
RDPSOBJ  = rdp-srvr.o tcp-s.o iso-s.o mcs-s.o secure-s.o licence.o rdp-s.o orders.o bitmap.o cache.o decompress.o xwin-s.o rfb.o vnc.o vncauth.o d3des.o
GNUOBJ   = gnu/getopt.o gnu/getopt1.o

#default to builtin headers and objects.
SSL_OBJ = crypto/rc4_enc.o crypto/rc4_skey.o crypto/md5_dgst.o crypto/sha1dgst.o crypto/bn_exp.o crypto/bn_mul.o crypto/bn_div.o crypto/bn_sqr.o crypto/bn_add.o crypto/bn_shift.o crypto/bn_asm.o crypto/bn_ctx.o crypto/bn_lib.o
SSL_INCLUDES = -I./crypto
SSL_LIBS =

#the xlib layer ( default )
GUI_XWIN_INC = -I/usr/X11R6/include
GUI_XWIN_LIB = -L/usr/X11R6/lib -lX11
GUI_XWIN_OBJ = gl/xwin.o

#the nt layer
GUI_WIN32_INC =
GUI_WIN32_LIB =
GUI_WIN32_OBJ = gl/mswin.o

#the svgalib layer
GUI_SVGA_INC =
GUI_SVGA_LIB = -lvga -lvgagl
GUI_SVGA_OBJ = gl/svga.o

#the AmigaOS layer
GUI_AMIGA_INC =
GUI_AMIGA_LIB = 
GUI_AMIGA_OBJ = gl/amiga.o

GUI_INCLUDES = ${GUI_XWIN_INC}
GUI_LIBS = ${GUI_XWIN_LIB}
GUI_OBJ = ${GUI_XWIN_OBJ}

OS = $(shell uname -s)
MACH = $(shell uname -m)

# add architecture specific stuff here.
ifeq "$(MACH)" "ppc"
CFLAGS += -fsigned-char
endif

# add system dependent libs etc
ifeq "$(OS)" "UnixWare"
LDLIBS += -l socket
endif
ifeq "$(OS)" "SunOS"
LDLIBS += -lsocket -lnsl -R$(X11DIR)/lib
endif
ifeq "$(OS)" "OSF1"
LDLIBS += -Wl,-rpath,$(X11DIR)/lib
endif
ifeq "$(OS)" "AIX"
CFLAGS += -I /usr/include/X11
LDLIBS += -L /usr/lib/X11
endif
ifeq "$(OS)" "IRIX"
CFLAGS += -fsigned-char -mips3 -mabi=n32
CFLAGS += -I /usr/freeware/include
LDLIBS += -L /usr/freeware/lib32
endif
ifeq "$(OS)" "IRIX64"
CFLAGS += -fsigned-char -mips3 -mabi=n32
CFLAGS += -I /usr/freeware/include
LDLIBS += -L /usr/freeware/lib32
endif

# if your system got getopt_long
# add it here, with correct libs etc.
ifeq "$(OS)" "Linux"
CFLAGS += -DLIBC_HAVE_GETOPT_LONG
GNUOBJ=
endif
ifeq "$(OS)" "AIX"
CFLAGS += -DLIBC_HAVE_GETOPT_LONG
GNUOBJ=
LDFLAGS += -liberty
endif


include Makeconf  # local configuration

all:	localendian.h rdesktop

localendian.h: tests/endian
	rm -f localendian.h
	./tests/endian > localendian.h

endian: tests/endian.c
	$(CC) -o tests/endian tests/endian.c

rdesktop: $(RDPOBJ) $(SSL_OBJ) ${GUI_OBJ} ${GNUOBJ}
	$(CC) $(CFLAGS) -o rdesktop $(RDPOBJ) $(SSL_OBJ) $(LDDIRS) $(LDLIBS) ${GNUOBJ} ${GUI_OBJ} ${GUI_LIBS} ${SSL_LIBS}

rdp-srvr: $(RDPSOBJ) ${SSL_OBJ} ${GUI_OBJ} $(GNUOBJ)
	$(CC) $(CFLAGS) -o rdp-srvr $(RDPSOBJ) $(SSL_OBJ) $(LDDIRS) $(LDLIBS) ${GNUOBJ} ${GUI_OBJ} ${GUI_LIBS}  ${SSL_LIBS}

Makeconf:
	./configure

install: installbin

installbin: rdesktop rdp-srvr
	mkdir -p $(BINDIR)
	cp rdesktop $(BINDIR)
	cp rdp-srvr $(BINDIR)
	strip $(BINDIR)/rdesktop
	strip $(BINDIR)/rdp-srvr
	chmod 755 $(BINDIR)/rdesktop
	chmod 755 $(BINDIR)/rdp-srvr

installman: rdesktop.1
	mkdir -p $(MANDIR)/man1
	cp rdesktop.1 $(MANDIR)/man1
	chmod 755 $(MANDIR)/man1/rdesktop.1

proto:
	cproto -DMAKE_PROTO -o proto.h *.c

clean:
	rm -f tests/endian localendian.h *.o gnu/*.o *.out crypto/*.o rdesktop rdp-srvr core
	rm -f crypto/*~ gnu/*~ tests/*~ *~ gl/*~ gl/ntxlib/*~
	rm -f gl/*.o

rdp-srvr.o: rdesktop.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

iso-s.o: iso.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

mcs-s.o: mcs.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

rdp-s.o: rdp.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

secure-s.o: secure.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

tcp-s.o: tcp.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

xwin-s.o: xwin.c keymap_wrap.c keymap.c
	$(CC) $(CFLAGS) -DSERVER -o $@ -c $<

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $(SSL_INCLUDES) $(GUI_INCLUDES) $(INCLUDES) -o $@ -c $<

