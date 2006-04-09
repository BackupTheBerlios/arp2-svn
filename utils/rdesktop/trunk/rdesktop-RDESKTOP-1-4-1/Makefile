#
# rdesktop: A Remote Desktop Protocol client
# Makefile.in
# Copyright (C) Matthew Chapman 1999-2004
#

prefix      = /usr/local
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin
mandir      = ${prefix}/man
datadir     = ${prefix}/share

VERSION     = 1.3.1
KEYMAP_PATH = $(datadir)/rdesktop/keymaps/

CC          = gcc
INSTALL     = /usr/bin/install -c
CFLAGS      = -g -O2 -Wall -I/usr/include  -I/usr/X11R6/include -DSTDC_HEADERS=1 -DHAVE_SYS_SELECT_H=1 -Dssldir=\"/usr\" -DEGD_SOCKET=\"/var/run/egd-pool\" -DWITH_RDPSND=1 -DHAVE_DIRENT_H=1 -DHAVE_DIRFD=1 -DHAVE_DECL_DIRFD=1  -DKEYMAP_PATH=\"$(KEYMAP_PATH)\"
LDFLAGS     =   -L/usr/lib -lcrypto  -L/usr/X11R6/lib 

TARGETS     = rdesktop 
VNCINC      = 
LDVNC       = 
VNCLINK     = 
SOUNDOBJ    = rdpsnd.o rdpsnd_oss.o

RDPOBJ   = tcp.o iso.o mcs.o secure.o licence.o rdp.o orders.o bitmap.o cache.o rdp5.o channels.o rdpdr.o serial.o printer.o disk.o parallel.o printercache.o mppc.o pstcache.o
X11OBJ   = rdesktop.o xwin.o xkeymap.o ewmhints.o xclip.o cliprdr.o
VNCOBJ   = vnc/rdp2vnc.o vnc/vnc.o vnc/xkeymap.o vnc/x11stubs.o

.PHONY: all
all: $(TARGETS)

rdesktop: $(X11OBJ) $(SOUNDOBJ) $(RDPOBJ)
	$(CC) $(CFLAGS) -o rdesktop $(X11OBJ) $(SOUNDOBJ) $(RDPOBJ) $(LDFLAGS) -lX11

rdp2vnc: $(VNCOBJ) $(SOUNDOBJ) $(RDPOBJ) 
	$(VNCLINK) $(CFLAGS) -o rdp2vnc $(VNCOBJ) $(SOUNDOBJ) $(RDPOBJ) $(LDFLAGS) $(LDVNC)

vnc/rdp2vnc.o: rdesktop.c
	$(CC) $(CFLAGS) $(VNCINC) -DRDP2VNC -o vnc/rdp2vnc.o -c rdesktop.c

vnc/vnc.o: vnc/vnc.c
	$(CC) $(CFLAGS) $(VNCINC) -DRDP2VNC -o vnc/vnc.o -c vnc/vnc.c

vnc/xkeymap.o: xkeymap.c
	$(CC) $(CFLAGS) $(VNCINC) -DRDP2VNC -o vnc/xkeymap.o -c xkeymap.c

vnc/x11stubs.o: vnc/x11stubs.c
	$(CC) $(CFLAGS) $(VNCINC) -o vnc/x11stubs.o -c vnc/x11stubs.c

.PHONY: install
install: installbin installkeymaps installman

.PHONY: installbin
installbin: rdesktop
	mkdir -p $(DESTDIR)/$(bindir)
	$(INSTALL) rdesktop $(DESTDIR)/$(bindir)
	strip $(DESTDIR)/$(bindir)/rdesktop
	chmod 755 $(DESTDIR)/$(bindir)/rdesktop

.PHONY: installman
installman: doc/rdesktop.1
	mkdir -p $(DESTDIR)/$(mandir)/man1
	cp doc/rdesktop.1 $(DESTDIR)/$(mandir)/man1
	chmod 644 $(DESTDIR)/$(mandir)/man1/rdesktop.1

.PHONY: installkeymaps
installkeymaps:
	mkdir -p $(DESTDIR)/$(KEYMAP_PATH)
# Prevent copying the CVS directory
	cp keymaps/?? keymaps/??-?? $(DESTDIR)/$(KEYMAP_PATH)
	cp keymaps/common $(DESTDIR)/$(KEYMAP_PATH)
	cp keymaps/modifiers $(DESTDIR)/$(KEYMAP_PATH)
	chmod 644 $(DESTDIR)/$(KEYMAP_PATH)/*

.PHONY: proto
proto:
	cproto -DMAKE_PROTO -DWITH_OPENSSL -o proto.h *.c

.PHONY: clean
clean:
	rm -f *.o *~ vnc/*.o vnc/*~ rdesktop rdp2vnc

.PHONY: dist
dist: rdesktop-$(VERSION).tar.gz

rdesktop-$(VERSION).tar.gz: Makefile configure
	mkdir -p /tmp/rdesktop-make-dist-dir
	ln -sf `pwd` /tmp/rdesktop-make-dist-dir/rdesktop-$(VERSION)
	(cd /tmp/rdesktop-make-dist-dir; \
	tar zcvf rdesktop-$(VERSION)/rdesktop-$(VERSION).tar.gz \
	rdesktop-$(VERSION)/COPYING \
	rdesktop-$(VERSION)/README \
	rdesktop-$(VERSION)/configure \
	rdesktop-$(VERSION)/configure.ac \
	rdesktop-$(VERSION)/config.sub \
	rdesktop-$(VERSION)/config.guess \
	rdesktop-$(VERSION)/bootstrap \
	rdesktop-$(VERSION)/install-sh \
	rdesktop-$(VERSION)/Makefile.in \
	rdesktop-$(VERSION)/rdesktop.spec \
	rdesktop-$(VERSION)/*.c \
	rdesktop-$(VERSION)/*.h \
	rdesktop-$(VERSION)/keymaps/?? \
	rdesktop-$(VERSION)/keymaps/??-?? \
	rdesktop-$(VERSION)/keymaps/common \
	rdesktop-$(VERSION)/keymaps/modifiers \
	rdesktop-$(VERSION)/keymaps/convert-map \
	rdesktop-$(VERSION)/doc/HACKING \
	rdesktop-$(VERSION)/doc/AUTHORS \
	rdesktop-$(VERSION)/doc/TODO \
	rdesktop-$(VERSION)/doc/ChangeLog \
	rdesktop-$(VERSION)/doc/keymapping.txt \
	rdesktop-$(VERSION)/doc/keymap-names.txt \
	rdesktop-$(VERSION)/doc/ipv6.txt \
	rdesktop-$(VERSION)/doc/licensing.txt \
	rdesktop-$(VERSION)/doc/patches.txt \
	rdesktop-$(VERSION)/doc/redirection.txt \
	rdesktop-$(VERSION)/doc/rdesktop.1 )
	rm -rf /tmp/rdesktop-make-dist-dir

.PHONY: dist-noversion
dist-noversion: rdesktop.tar.gz

rdesktop.tar.gz: rdesktop-$(VERSION).tar.gz
	mkdir -p /tmp/rdesktop-make-dist-dir
	tar zxvf $< -C /tmp/rdesktop-make-dist-dir
	mv /tmp/rdesktop-make-dist-dir/rdesktop-$(VERSION) /tmp/rdesktop-make-dist-dir/rdesktop
	ls /tmp/rdesktop-make-dist-dir/rdesktop
	tar zcvf $@ -C /tmp/rdesktop-make-dist-dir rdesktop
	rm -rf /tmp/rdesktop-make-dist-dir

Makefile: Makefile.in configure
	./config.status

configure: configure.ac
	./bootstrap

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

