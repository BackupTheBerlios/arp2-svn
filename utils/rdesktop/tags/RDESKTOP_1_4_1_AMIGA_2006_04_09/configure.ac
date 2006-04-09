# Process this file with autoconf to produce a configure script.
AC_INIT(amiga_rdesktop, 1.4.1-2006-04-09-amiga, martin@blom.org)
AC_CONFIG_SRCDIR([amiga_rdesktop.c])
AC_CONFIG_HEADER([config.h])

AC_MSG_CHECKING(if we should use libnix instead of ixemul.library)
AC_ARG_WITH(libnix,
[  --with-libnix           Use libnix instead of ixemul.library],
[libnix='-noixemul' ALIBS=''],
[libnix='' ALIBS=', ixemul V48' withval=no])
AC_MSG_RESULT(${withval})

AC_CANONICAL_HOST

case $host in
 m68k-unknown-amigaos)
  OS="AmigaOS"
  OS_VERSION="AmigaOS 3.x or OS4 (emulated)"
  OS_VERSION_AGA="AmigaOS 3.x"
  OS_VERSION_GFX="AmigaOS 3.x+P96/CGFX"
  DESKTOP="Workbench"
  CPU="68020"
 ;;
 powerpc-unknown-amigaos)
  OS="AmigaOS 4"
  OS_VERSION="AmigaOS 4"
  OS_VERSION_AGA="AmigaOS 4"
  OS_VERSION_GFX="AmigaOS 4 (P96)"
  DESKTOP="Workbench"
  CPU="PowerPC 603"
 ;;
 powerpc-unknown-morphos)
  OS="MorphOS"
  OS_VERSION="MorphOS 0.4/1.4+"
  OS_VERSION_AGA="MorphOS 0.4 (untested)"
  OS_VERSION_GFX="MorphOS 0.4/1.4+"
  DESKTOP="Ambient"
  CPU="PowerPC 603"
 ;;
 *)
  AC_MSG_ERROR(Unknown architecture!)
 ;;
esac

AC_SUBST(ALIBS)
AC_SUBST(OS)
AC_SUBST(OS_VERSION)
AC_SUBST(OS_VERSION_AGA)
AC_SUBST(OS_VERSION_GFX)
AC_SUBST(DESKTOP)
AC_SUBST(CPU)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
#AC_PROG_LN_S

if test "x${GCC}" = "xyes"; then
  : ${CPPFLAGS:="-W -Wall -Wno-unused"}
  : ${CFLAGS:="-g -O2"}
  : ${LDFLAGS:="-s"}
fi

# Add -noixemul if requested
CPPFLAGS="${CPPFLAGS} ${libnix}"
LDFLAGS="${LDFLAGS} ${libnix}"

# Checks for libraries.
#AC_SEARCH_LIBS([floor], [m])
AC_SEARCH_LIBS([getuid], [socket])
AC_CHECK_LIB([crypto], [SHA1_Init])

# Checks for header files.
#AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
#AC_CHECK_HEADERS([alloca.h arpa/inet.h errno.h fcntl.h limits.h mntent.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/ioctl.h sys/mntent.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h sys/vfs.h termios.h unistd.h utime.h])
AC_CHECK_HEADERS([newmouse.h devices/newmouse.h devices/rawkeycodes.h])
			    
# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN([AC_DEFINE([B_ENDIAN], [], [Big endian machine?])], 
               [AC_DEFINE([L_ENDIAN], [], [Little endian machine?])])
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
#AC_FUNC_ALLOCA
#AC_FUNC_ERROR_AT_LINE
#AC_FUNC_FNMATCH
#AC_FUNC_FORK
#AC_PROG_GCC_TRADITIONAL
#AC_FUNC_MALLOC
#AC_FUNC_MKTIME
#AC_FUNC_STAT
#AC_FUNC_UTIME_NULL
#AC_FUNC_VPRINTF
#AC_CHECK_FUNCS([bzero gethostbyname gethostname getmntent gettimeofday memset mkdir rmdir select socket strchr strerror strrchr strspn strstr strtol utime])

AC_CONFIG_FILES([RDesktop.readme Makefile],[echo timestamp > stamp-h])
AC_OUTPUT
