ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = config.h m4 amiga docs CHANGES README

SUBDIRS = src

noinst_HEADERS = config.h
dist_noinst_SCRIPTS = bootstrap.sh

dist-hook:
	rm -rf `find $(distdir) -name CVS`
	rm -rf `find $(distdir) -name *.bak`
	rm -rf `find $(distdir) -name *~`
	rm -rf `find $(distdir) -name *.orig`
