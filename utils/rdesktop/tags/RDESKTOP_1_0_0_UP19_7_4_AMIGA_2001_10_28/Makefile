##############################################
# rdesktop: A Remote Desktop Protocol client #
# Basic Makefile                             #
# Copyright (C) Matthew Chapman 1999-2000    #
##############################################

# Uncomment to enable debugging
# DEBUG = -g -DRDP_DEBUG
# DEBUG = -DDEBUG -DX_SYNCED
# DEBUG = -g -DDEBUG_LEAK
# DEBUG = -g -pg
# DEBUG = -g

# Xinerama flags
#XINERAMA_CFLAGS = -DHAVE_XINERAMA
#XINERAMA_LIBS = -lXinerama -lXext

PREFIX = /usr/local
X11DIR = /usr/X11R6

CC     = gcc
COFLAGS = -O2 -m68020-60 -funroll-loops -fexpensive-optimizations -ffast-math -fomit-frame-pointer -frerun-cse-after-loop
CFLAGS = $(COFLAGS) -Wall $(DEBUG) #$(XINERAMA_CFLAGS) -I$(X11DIR)/include
LDFLAGS   = -lgmp -lcrypto #-L$(X11DIR)/lib -lX11 $(XINERAMA_LIBS) #-lmp
#-c flag doesn't seem to be needed at gnu platforms
#it's only ignored. but at others the manual file will be lost if it's not used.
INSTALL= install -c -m 755 -s
INSTALLMAN= install -c -m 644

OS = $(shell uname -s)
MACH = $(shell uname -m)

# add architecture specific stuff here.
ifeq "$(MACH)" "ppc"
COFLAGS += -fsigned-char
endif

# add system dependent libs etc
ifeq "$(OS)" "UnixWare"
LDFLAGS += -l socket
endif
ifeq "$(OS)" "SunOS"
LDFLAGS += -lsocket -lnsl -R$(X11DIR)/lib
endif
ifeq "$(OS)" "OSF1"
LDFLAGS += -Wl,-rpath,$(X11DIR)/lib
endif
ifeq "$(OS)" "AIX"
CFLAGS += -I /usr/include/X11
LDFLAGS += -L /usr/lib/X11
endif
ifeq "$(OS)" "IRIX"
COFLAGS += -fsigned-char -mips3 -mabi=n32
CFLAGS += -I /usr/freeware/include -I /usr/include/sys
LDFLAGS += -L /usr/freeware/lib32
endif
ifeq "$(OS)" "IRIX64"
COFLAGS += -fsigned-char -mips3 -mabi=n32
CFLAGS += -I /usr/freeware/include  -I /usr/include/sys
LDFLAGS += -L /usr/freeware/lib32
endif

HAVE_GETOPT = 0

# if your system got getopt_long
# add it here, with correct libs etc.
ifeq "$(OS)" "Linux"
HAVE_GETOPT = 1
endif
ifeq "$(OS)" "AIX"
HAVE_GETOPT = 1
LDFLAGS += -liberty
endif

ifeq "$(HAVE_GETOPT)" "1"
CFLAGS += -DLIBC_HAVE_GETOPT_LONG
else
GNUOBJ = gnu/getopt.o gnu/getopt1.o
endif


#RDPOBJ = rdesktop.o tcp.o iso.o mcs.o secure.o licence.o rdp.o orders.o bitmap.o cache.o xwin.o
RDPOBJ = rdesktop_amiga.o tcp.o iso.o mcs.o secure.o licence.o rdp.o orders.o bitmap.o cache.o amiga.o
RDPSOBJ = rdp-srvr.o tcp-s.o iso-s.o mcs-s.o secure-s.o licence.o rdp-s.o orders.o bitmap.o cache.o xwin-s.o rfb.o vnc.o vncauth.o d3des.o

#all:	localendian.h rdesktop rdp-srvr
all:	localendian.h rdesktop

localendian.h: endian
	rm -f localendian.h
	./endian > localendian.h

endian: endian.c
	$(CC) -o endian endian.c

rdesktop: $(GNUOBJ) $(RDPOBJ)
	$(CC) $(CFLAGS) -o rdesktop $(RDPOBJ) $(GNUOBJ) $(LDFLAGS)

rdp-srvr: $(GNUOBJ) $(RDPSOBJ)
	$(CC) $(CFLAGS) -o rdp-srvr $(RDPSOBJ) $(GNUOBJ) $(LDFLAGS)

proto:
	cproto -DMAKE_PROTO -o proto.h *.c

clean:
	rm -f endian localendian.h *.o gnu/*.o *~ *.out rdesktop rdp-srvr core

#genkb: genkb.o
#	$(CC) $(CFLAGS) -o genkb $(LDFLAGS) genkb.o
#	./genkb > kbcodes.h

xwin.o: xwin.c keymap_wrap.c

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
	$(CC) $(CFLAGS) -o $@ -c $<

install: rdesktop rdp-srvr
	$(INSTALL) rdesktop $(PREFIX)/bin
	$(INSTALL) rdp-srvr $(PREFIX)/bin
	$(INSTALLMAN) rdesktop.1 $(PREFIX)/man/man1


