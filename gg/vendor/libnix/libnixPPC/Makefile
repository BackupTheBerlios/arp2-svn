SOURCES = __nocommandline.c stdio/printf.c stdio/vfprintf.c stdio/vprintf.c \
	stdio/__stdio.c stdio/fopen.c stdlib/calloc.c stdlib/malloc.c \
	stdio/freopen.c stdlib/free.c errno/errno.c stdio/__fflush.c \
	stdio/open.c errno/__seterrno.c ctype/_ctype_.c stdio/fputc.c \
	stdio/__swbuf.c stdlib/realloc.c signal/__chkabort.c \
	signal/raise.c signal/signal.c signal/__signalfunc.c \
	stdio/isinf.c stdio/isnan.c locale/__decimalpoint.c \
	stdio/scanf.c stdio/vscanf.c stdio/vfscanf.c stdio/fgetc.c \
	stdio/ungetc.c stdio/__srget.c assert/__eprintf.c \
	ctype/isalnum.c ctype/isalpha.c ctype/iscntrl.c \
	ctype/isdigit.c ctype/isgraph.c ctype/islower.c \
	ctype/isprint.c ctype/ispunct.c ctype/isspace.c \
	ctype/isupper.c ctype/isxdigit.c ctype/tolower.c \
	ctype/toupper.c setjmp/longjmp.c setjmp/setjmp.c \
	string/memset.c math/ceil.c time/time.c time/difftime.c \
	stdio/fread.c string/strerror.c stdio/ftell.c stdio/fseek.c \
	stdio/fwrite.c time/__gmtoffset.c locale/localeconv.c \
	locale/__lconv.c locale/__localevec.c locale/setlocale.c \
	math/acos.c math/asin.c math/cosh.c math/fabs.c math/floor.c \
	math/fmod.c math/modf.c math/frexp.c math/ldexp.c math/sinh.c \
	math/tan.c math/tanh.c stdlib/atof.c stdlib/strtod.c \
	stdlib/abort.c stdlib/abs.c stdlib/atexit.c stdlib/atoi.c \
	stdlib/atol.c stdlib/bsearch.c stdlib/labs.c stdlib/qsort.c \
	stdlib/rand.c stdlib/strtol.c stdlib/strtoul.c stdlib/getenv.c \
	stdlib/system.c time/asctime.c time/strftime.c time/localtime.c \
	time/gmtime.c time/clock.c time/mktime.c stdlib/div.c \
	stdlib/ldiv.c stdio/clearerr.c stdio/feof.c stdio/ferror.c \
	stdio/fflush.c stdio/fgetpos.c stdio/fgets.c stdio/fileno.c \
	stdio/fprintf.c stdio/fputs.c stdio/fscanf.c stdio/fsetpos.c \
	stdio/gets.c string/memchr.c string/memcmp.c string/memmove.c \
	string/bcopy.c stdio/mblen.c stdio/mbstowcs.c stdio/mbtowc.c \
	stdio/perror.c stdio/puts.c stdio/remove.c stdio/rename.c \
	stdio/rewind.c stdio/setbuf.c stdio/setvbuf.c stdio/sprintf.c \
	stdio/sscanf.c string/strcat.c string/strchr.c string/strcmp.c \
	string/strcpy.c string/strcspn.c string/strlen.c string/strncat.c \
	string/strncmp.c string/strncpy.c string/strpbrk.c string/strrchr.c \
	string/strspn.c string/strstr.c string/strtok.c stdio/tmpfile.c \
	stdio/tmpnam.c stdio/vsprintf.c stdio/wcstombs.c stdio/wctomb.c \
	stdio/vsscanf.c extra/stricmp.c extra/getcwd.c extra/chdir.c \
	extra/dirent.c extra/stat.c extra/__stat.c extra/access.c \
	extra/fstat.c string/bzero.c extra/index.c stdio/__amigapath.c \
	extra/strdup.c extra/rindex.c extra/strchr.c extra/strrchr.c \
	extra/dotimer.c extra/timer.c extra/gettimeofday.c extra/strnicmp.c

STARTUPSRC = startup.c

MOTOOBJS = math/pow.emb.o math/trig.emb.o math/sqrt.emb.o math/atan.emb.o

ENDSRC = end.c

TESTSRC = test.c

ARCHIVE = libc.a

OTHER = 

CC = ppc-amigaos-gcc#-V egcs-2.91.66

LD = ppc-amigaos-ld

AR = ppc-amigaos-ar

RANLIB = ppc-amigaos-ranlib

CODEGEN = -DIXPATHS -mcpu=604e -Wall -I. -mmultiple

OPTIMIZE = -O3 -funroll-loops

CFLAGS= $(CODEGEN) $(OPTIMIZE)

LIBS =

OBJS = $(SOURCES:.c=.o)

STARTUPOBJ = $(STARTUPSRC:.c=.o)

ENDOBJ = $(ENDSRC:.c=.o)

TESTOBJS = $(TESTSRC:.c=.o)

ASMS = $(SOURCES:.c=.s)

STARTUPASM = $(STARTUPSRC:.c=.s)

ENDASM = $(ENDSRC:.c=.s)

TESTASMS = $(TESTSRC:.c=.s)

TESTELFS = $(TESTSRC:.c=.elf)

all : $(ARCHIVE) $(STARTUPOBJ) $(ENDOBJ)#$(TESTELFS) $(TESTOBJS)

%.elf : %.o $(STARTUPOBJ) $(ARCHIVE) $(ENDOBJ)
	$(LD) -r $(STARTUPOBJ) $< $(ARCHIVE) $(ENDOBJ) -o $@
	protect $@ e add

%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o

$(ARCHIVE) : $(OBJS) $(MOTOOBJS)
	$(AR) r $@ $?
	$(RANLIB) $@
#	copy libc.a ade:ppc-amigaos/lib

depend : 
	mkdepend $(SOURCES) $(TESTSRC) -fMakefile

clean :
	rm -f $(ARCHIVE) $(OBJS) $(ASMS) $(OTHER) $(STARTUPOBJ) \
	$(STARTUPASM) $(TESTELFS) $(TESTASMS) $(TESTOBJS)\
	$(ENDOBJ) $(ENDASM)
# --- DO NOT MODIFY THIS LINE -- AUTO-DEPENDS FOLLOW ---
__nocommandline.o : stabs.h

assert/__eprintf.o :

ctype/_ctype_.o :

ctype/isalnum.o :

ctype/isalpha.o :

ctype/iscntrl.o :

ctype/isdigit.o :

ctype/isgraph.o :

ctype/islower.o :

ctype/isprint.o :

ctype/ispunct.o :

ctype/isspace.o :

ctype/isupper.o :

ctype/isxdigit.o :

ctype/tolower.o :

ctype/toupper.o :

errno/__seterrno.o :

errno/errno.o :

extra/__stat.o :

extra/access.o :

extra/chdir.o :

extra/dirent.o : dirent.h

extra/dotimer.o :

extra/fstat.o :

extra/getcwd.o :

extra/gettimeofday.o :

extra/index.o :

extra/rindex.o :

extra/stat.o :

extra/strchr.o :

extra/strdup.o :

extra/stricmp.o :

extra/strnicmp.o :

extra/strrchr.o :

extra/timer.o :

locale/__decimalpoint.o :

locale/__lconv.o :

locale/__localevec.o :

locale/localeconv.o :

locale/setlocale.o :

math/acos.o :

math/asin.o :

math/ceil.o :

math/cosh.o :

math/fabs.o :

math/floor.o :

math/fmod.o :

math/frexp.o :

math/ldexp.o :

math/modf.o :

math/sinh.o :

math/tan.o :

math/tanh.o :

setjmp/longjmp.o :

setjmp/setjmp.o :

signal/__chkabort.o :

signal/__signalfunc.o :

signal/raise.o :

signal/signal.o :

stdio/__amigapath.o :

stdio/__fflush.o :

stdio/__srget.o :

stdio/__stdio.o :

stdio/__swbuf.o :

stdio/clearerr.o :

stdio/feof.o :

stdio/ferror.o :

stdio/fflush.o :

stdio/fgetc.o :

stdio/fgetpos.o :

stdio/fgets.o :

stdio/fileno.o :

stdio/fopen.o :

stdio/fprintf.o :

stdio/fputc.o :

stdio/fputs.o :

stdio/fread.o :

stdio/freopen.o :

stdio/fscanf.o :

stdio/fseek.o :

stdio/fsetpos.o :

stdio/ftell.o :

stdio/fwrite.o :

stdio/gets.o :

stdio/isinf.o :

stdio/isnan.o :

stdio/mblen.o :

stdio/mbstowcs.o :

stdio/mbtowc.o :

stdio/open.o :

stdio/perror.o :

stdio/printf.o :

stdio/puts.o :

stdio/remove.o :

stdio/rename.o :

stdio/rewind.o :

stdio/scanf.o :

stdio/setbuf.o :

stdio/setvbuf.o :

stdio/sprintf.o :

stdio/sscanf.o :

stdio/tmpfile.o :

stdio/tmpnam.o :

stdio/ungetc.o :

stdio/vfprintf.o :

stdio/vfscanf.o :

stdio/vprintf.o :

stdio/vscanf.o :

stdio/vsprintf.o :

stdio/vsscanf.o :

stdio/wcstombs.o :

stdio/wctomb.o :

stdlib/abort.o :

stdlib/abs.o :

stdlib/atexit.o :

stdlib/atof.o :

stdlib/atoi.o :

stdlib/atol.o :

stdlib/bsearch.o :

stdlib/calloc.o :

stdlib/div.o :

stdlib/free.o :

stdlib/getenv.o :

stdlib/labs.o :

stdlib/ldiv.o :

stdlib/malloc.o :

stdlib/qsort.o :

stdlib/rand.o :

stdlib/realloc.o :

stdlib/strtod.o :

stdlib/strtol.o :

stdlib/strtoul.o :

stdlib/system.o :

string/bcopy.o :

string/bzero.o :

string/memchr.o :

string/memcmp.o :

string/memmove.o :

string/memset.o :

string/strcat.o :

string/strchr.o :

string/strcmp.o :

string/strcpy.o :

string/strcspn.o :

string/strerror.o :

string/strlen.o :

string/strncat.o :

string/strncmp.o :

string/strncpy.o :

string/strpbrk.o :

string/strrchr.o :

string/strspn.o :

string/strstr.o :

string/strtok.o :

test.o :

time/__gmtoffset.o :

time/asctime.o :

time/clock.o :

time/difftime.o :

time/gmtime.o :

time/localtime.o :

time/mktime.o :

time/strftime.o :

time/time.o :

# --- DO NOT MODIFY THIS LINE -- AUTO-DEPENDS PRECEDE ---
