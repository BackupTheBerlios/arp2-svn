# Process this file with autoconf to produce a configure script.
AC_INIT(amiga_rdesktop, cvs-2004-04-16-amiga-0.1, martin@blom.org)
AC_CONFIG_SRCDIR([amiga_rdesktop.c])
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
#AC_PROG_LN_S

if test "x${GCC}" = "xyes"; then
  : ${CPPFLAGS:="-W -Wall -Wno-unused"}
fi
	
# Checks for libraries.
#AC_SEARCH_LIBS([floor], [m])
AC_SEARCH_LIBS([getuid], [socket])
AC_CHECK_LIB([crypto], [SHA1_Init])

# Checks for header files.
#AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
#AC_CHECK_HEADERS([alloca.h arpa/inet.h errno.h fcntl.h limits.h mntent.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/ioctl.h sys/mntent.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h sys/vfs.h termios.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
