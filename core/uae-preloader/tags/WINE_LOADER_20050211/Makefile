TOPSRCDIR = ..
TOPOBJDIR = ..
SRCDIR    = .

MODULE    = wine

C_SRCS = \
	glibc.c \
	kthread.c \
	main.c \
	preloader.c \
	pthread.c

KTHREAD_OBJS = kthread.o main.o
PTHREAD_OBJS = pthread.o main.o

WINE_BINARIES = wine-glibc wine-kthread wine-pthread wine-preloader
MAIN_BINARY   = wine-glibc

all: $(WINE_BINARIES) $(MODULE)


# Global rules shared by all makefiles     -*-Makefile-*-
#
# Each individual makefile must define the following variables:
# TOPSRCDIR    : top-level source directory
# TOPOBJDIR    : top-level object directory
# SRCDIR       : source directory for this module
# MODULE       : name of the module being built
#
# Each individual makefile may define the following additional variables:
# C_SRCS       : C sources for the module
# C_SRCS16     : 16-bit C sources for the module
# RC_SRCS      : resource source files
# EXTRA_SRCS   : extra source files for make depend
# EXTRA_OBJS   : extra object files
# IMPORTS      : dlls to import
# DELAYIMPORTS : dlls to import in delayed mode
# SUBDIRS      : subdirectories that contain a Makefile
# EXTRASUBDIRS : subdirectories that do not contain a Makefile
# INSTALLSUBDIRS : subdirectories to run make install/uninstall into

# First some useful definitions

SHELL     = /bin/sh
CC        = gcc
CFLAGS    = -g -O2
CPPFLAGS  = 
LIBS      = 
BISON     = bison
YACC      = $(BISON) -y
LEX       = flex
LEXLIB    = -lfl
EXEEXT    = 
OBJEXT    = o
LIBEXT    = so
DLLEXT    = .so
IMPLIBEXT = def
LDSHARED  = $(CC) -shared $(SONAME:%=-Wl,-soname,%) $(VERSCRIPT:%=-Wl,--version-script=%)
DLLTOOL   = false
DLLWRAP   = 
AR        = ar rc
RANLIB    = ranlib
STRIP     = strip
WINDRES   = false
LN        = ln
LN_S      = ln -s
TOOLSDIR  = $(TOPOBJDIR)
AS        = as
LD        = ld
LDFLAGS   = 
RM        = rm -f
MV        = mv
LINT      = 
LINTFLAGS = 
FONTFORGE = false
INCLUDES     = -I$(SRCDIR) -I. -I$(TOPSRCDIR)/include -I$(TOPOBJDIR)/include $(EXTRAINCL)
EXTRACFLAGS  = -Wall -pipe -mpreferred-stack-boundary=2 -fno-strict-aliasing -gstabs+ -Wpointer-arith
ALLCFLAGS    = $(INCLUDES) $(DEFS) $(DLLFLAGS) $(EXTRACFLAGS) $(CPPFLAGS) $(CFLAGS)
ALLLINTFLAGS = $(INCLUDES) $(DEFS) $(LINTFLAGS)
IDLFLAGS     = $(INCLUDES) $(DEFS) $(EXTRAIDLFLAGS)
MKINSTALLDIRS= $(TOPSRCDIR)/tools/mkinstalldirs -m 755
WINAPI_CHECK = $(TOPSRCDIR)/tools/winapi_check/winapi_check
WINEWRAPPER  = $(TOPSRCDIR)/tools/winewrapper
C2MAN        = $(TOPSRCDIR)/tools/c2man.pl
RUNTEST      = $(TOPSRCDIR)/tools/runtest
WINEBUILD    = $(TOOLSDIR)/tools/winebuild/winebuild
MAKEDEP      = $(TOOLSDIR)/tools/makedep
WRC          = $(TOOLSDIR)/tools/wrc/wrc
BIN2RES      = $(TOOLSDIR)/tools/bin2res
WMC          = $(TOOLSDIR)/tools/wmc/wmc
WIDL         = $(TOOLSDIR)/tools/widl/widl
WINEGCC      = $(TOOLSDIR)/tools/winegcc/winegcc
SFNT2FNT     = $(TOOLSDIR)/tools/sfnt2fnt
FNT2FON      = $(TOOLSDIR)/tools/fnt2fon
RC           = $(WRC)
RC16         = $(WRC)
RCFLAGS      = --nostdinc $(INCLUDES) $(DEFS) $(EXTRARCFLAGS)
RC16FLAGS    = -O res16 $(RCFLAGS)
LDPATH       = LD_LIBRARY_PATH="$(TOOLSDIR)/libs/unicode:$$LD_LIBRARY_PATH"
DLLDIR       = $(TOPOBJDIR)/dlls
LIBDIR       = $(TOPOBJDIR)/libs
LIBPORT      = -L$(TOPOBJDIR)/libs/port -lwine_port
LIBUNICODE   = -L$(TOPOBJDIR)/libs/unicode -lwine_unicode
LIBWINE      = -L$(TOPOBJDIR)/libs/wine -lwine



# Installation infos

INSTALL         = /usr/bin/install -c $(INSTALL_FLAGS)
INSTALL_PROGRAM = ${INSTALL} $(INSTALL_PROGRAM_FLAGS)
INSTALL_SCRIPT  = ${INSTALL} $(INSTALL_SCRIPT_FLAGS)
INSTALL_DATA    = ${INSTALL} -m 644 $(INSTALL_DATA_FLAGS)
prefix          = /usr/local
exec_prefix     = ${prefix}
bindir          = ${exec_prefix}/bin
libdir          = ${exec_prefix}/lib
datadir         = ${prefix}/share
infodir         = ${prefix}/info
mandir          = ${prefix}/man
sysconfdir      = ${prefix}/etc
includedir      = ${prefix}/include/wine
dlldir          = ${exec_prefix}/lib/wine
prog_manext     = 1
api_manext      = 3w
conf_manext     = 5
CLEAN_FILES     = *.o *.a *.so *.ln *.$(LIBEXT) \\\#*\\\# *~ *% .\\\#* *.bak *.orig *.rej \
                  *.flc *.spec.c *.spec.def *.dbg.c *.tab.c *.tab.h lex.yy.c core

OBJS = $(C_SRCS:.c=.o) $(EXTRA_OBJS)

RCOBJS = $(RC_SRCS:.rc=.res.o)
LINTS  = $(C_SRCS:.c=.ln)

# Implicit rules

.SUFFIXES: .mc .rc .mc.rc .res .res.o .spec .spec.c .spec.def .idl .tlb .h .ok .sfd .ttf

.c.o:
	$(CC) -c $(ALLCFLAGS) -o $@ $<

.s.o:
	$(AS) -o $@ $<

.mc.mc.rc:
	$(LDPATH) $(WMC) -i -U -H /dev/null -o $@ $<

.rc.res:
	$(LDPATH) $(RC) $(RCFLAGS) -fo$@ $<

.res.res.o:
	$(WINDRES) -i $< -o $@

.spec.spec.c:
	$(WINEBUILD) $(DEFS) -o $@ --main-module $(MODULE) --dll $<

.spec.spec.def:
	$(WINEBUILD) -w $(DEFS) -o $@ --def $<

.idl.h:
	$(WIDL) $(IDLFLAGS) -h -H $@ $<

.idl.tlb:
	$(WIDL) $(IDLFLAGS) -t -T $@ $<

.c.ln:
	$(LINT) -c $(ALLLINTFLAGS) $< || ( $(RM) $@ && exit 1 )

.c.ok:
	$(RUNTEST) $(RUNTESTFLAGS) $< && touch $@

.sfd.ttf:
	$(FONTFORGE) -script $(TOPSRCDIR)/fonts/genttf.ff $<

# 'all' target first in case the enclosing Makefile didn't define any target

all: Makefile

filter:
	@$(TOPSRCDIR)/tools/winapi/make_filter --make $(MAKE) all

.PHONY: all filter

# Rules for resources

$(RC_BINARIES): $(BIN2RES) $(RC_BINSRC)
	$(BIN2RES) -f -o $@ $(SRCDIR)/$(RC_BINSRC)

$(RC_SRCS:.rc=.res) $(RC_SRCS16:.rc=.res): $(WRC) $(RC_BINARIES) $(RC_TLB)

# Rule for main module debug channels

$(MODULE).dbg.c: $(C_SRCS) $(C_SRCS16) $(WINEBUILD)
	$(WINEBUILD) $(DEFS) -o $@ --debug -C$(SRCDIR) $(C_SRCS) $(C_SRCS16)

# Rules for makefile

Makefile: Makefile.in $(TOPSRCDIR)/configure
	@echo Makefile is older than $?, please rerun $(TOPSRCDIR)/configure
	@exit 1

# Rule for linting

$(MODULE).ln : $(LINTS)
	if test "$(LINTS)" ; \
	then \
		$(LINT) $(ALLLINTFLAGS) -o$(MODULE) $(LINTS) ; \
	        $(MV) llib-l$(MODULE).ln $(MODULE).ln ; \
	else \
		$(LINT) $(ALLLINTFLAGS) -C$(MODULE) /dev/null ; \
	fi

lint:: $(MODULE).ln

# Rules for Windows API checking

winapi_check:: dummy
	$(WINAPI_CHECK) $(WINAPI_CHECK_FLAGS) $(WINAPI_CHECK_EXTRA_FLAGS) .

.PHONY: winapi_check

# Rules for dependencies

$(SUBDIRS:%=%/__depend__): dummy
	cd `dirname $@` && $(MAKE) depend

depend: $(IDL_SRCS:.idl=.h) $(SUBDIRS:%=%/__depend__)
	$(MAKEDEP) $(INCLUDES) -C$(SRCDIR) $(C_SRCS) $(C_SRCS16) $(RC_SRCS) $(RC_SRCS16) $(MC_SRCS) $(IDL_SRCS) $(EXTRA_SRCS)

.PHONY: depend $(SUBDIRS:%=%/__depend__)

# Rules for cleaning

$(SUBDIRS:%=%/__clean__): dummy
	cd `dirname $@` && $(MAKE) clean

$(SUBDIRS:%=%/__testclean__): dummy
	cd `dirname $@` && $(MAKE) testclean

$(EXTRASUBDIRS:%=%/__clean__): dummy
	-cd `dirname $@` && $(RM) $(CLEAN_FILES)

testclean:: $(SUBDIRS:%=%/__testclean__)

clean:: $(SUBDIRS:%=%/__clean__) $(EXTRASUBDIRS:%=%/__clean__)
	$(RM) $(CLEAN_FILES) $(RC_SRCS:.rc=.res) $(RC_SRCS16:.rc=.res) $(MC_SRCS:.mc=.mc.rc) $(IDL_SRCS:.idl=.h) $(PROGRAMS) $(RC_BINARIES) $(RC_TLB)

.PHONY: clean testclean $(SUBDIRS:%=%/__clean__) $(SUBDIRS:%=%/__testclean__) $(EXTRASUBDIRS:%=%/__clean__)

# Rules for installing

$(SUBDIRS:%=%/__install__): dummy
	cd `dirname $@` && $(MAKE) install

$(SUBDIRS:%=%/__install-lib__): dummy
	cd `dirname $@` && $(MAKE) install-lib

$(SUBDIRS:%=%/__install-dev__): dummy
	cd `dirname $@` && $(MAKE) install-dev

$(SUBDIRS:%=%/__uninstall__): dummy
	cd `dirname $@` && $(MAKE) uninstall

install:: $(INSTALLSUBDIRS:%=%/__install__)

uninstall:: $(INSTALLSUBDIRS:%=%/__uninstall__)

.PHONY: install install-lib install-dev uninstall \
	$(SUBDIRS:%=%/__install__) $(SUBDIRS:%=%/__uninstall__) \
	$(SUBDIRS:%=%/__install-lib__) $(SUBDIRS:%=%/__install-dev__)

# Rules for checking that no imports are missing

$(SUBDIRS:%=%/__checklink__): dummy
	@cd `dirname $@` && $(MAKE) checklink

.PHONY: checklink $(SUBDIRS:%=%/__checklink__)

# Rules for testing

$(SUBDIRS:%=%/__test__): dummy
	@cd `dirname $@` && $(MAKE) test

$(SUBDIRS:%=%/__crosstest__): dummy
	@cd `dirname $@` && $(MAKE) crosstest

.PHONY: check test crosstest $(SUBDIRS:%=%/__test__) $(SUBDIRS:%=%/__crosstest__)

# Misc. rules

$(MC_SRCS:.mc=.mc.rc): $(WMC)

$(IDL_SRCS:.idl=.h): $(WIDL)

$(RC_TLB:.idl=.tlb): $(WIDL)

$(SUBDIRS): dummy
	@cd $@ && $(MAKE)

dummy:

.PHONY: dummy $(SUBDIRS)

# End of global rules

LIBPTHREAD  = -lpthread
LDEXECFLAGS = -Wl,--export-dynamic -Wl,--section-start,.interp=0x77f00400

wine-glibc: glibc.o Makefile.in
	$(CC) -o $@ glibc.o $(LIBWINE) $(LIBPORT) $(LIBPTHREAD) $(EXTRALIBS) $(LDFLAGS)

wine-preloader: preloader.o Makefile.in
	$(CC) -o $@ -static -nostartfiles -nodefaultlibs -Wl,-Ttext=0x78000000 preloader.o $(LIBPORT) $(LDFLAGS)

wine-kthread: $(KTHREAD_OBJS) Makefile.in
	$(CC) -o $@ $(LDEXECFLAGS) $(KTHREAD_OBJS) $(LIBWINE) $(LIBPORT) $(EXTRALIBS) $(LDFLAGS)

wine-pthread: $(PTHREAD_OBJS) Makefile.in
	$(CC) -o $@ $(LDEXECFLAGS) $(PTHREAD_OBJS) $(LIBWINE) $(LIBPORT) $(LIBPTHREAD) $(EXTRALIBS) $(LDFLAGS)

$(MODULE): $(MAIN_BINARY)
	$(RM) $(MODULE) && $(LN_S) $(MAIN_BINARY) $(MODULE)

install:: $(WINE_BINARIES)
	$(MKINSTALLDIRS) $(bindir)
	for f in $(WINE_BINARIES); do \
	  if [ "$(MAIN_BINARY)" = "$$f" ]; \
	  then $(INSTALL_PROGRAM) $$f $(bindir)/$(MODULE); \
	  else $(INSTALL_PROGRAM) $$f $(bindir)/$$f; \
	  fi; \
	done

uninstall::
	-cd $(bindir) && $(RM) $(WINE_BINARIES) $(MODULE)

clean::
	$(RM) $(WINE_BINARIES) $(MODULE)

### Dependencies:
