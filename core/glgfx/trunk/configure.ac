#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([glgfx], [0.1], [martin@blom.org])
AM_CONFIG_HEADER(glgfx-config.h)
AM_INIT_AUTOMAKE([glgfx], [0.1])
AC_CONFIG_SRCDIR([src/glgfx.c])

AC_PROG_MAKE_SET

: ${CFLAGS="-W -Wall"}
	
# Checks for programs.
AC_CHECK_TOOL([STRIP],[strip])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
	
AM_DISABLE_SHARED
AM_PROG_LIBTOOL

# X11
AC_PATH_XTRA
if test "x${X_DISPLAY_MISSING}" != "x"; then
   AC_ERROR([X11 is required])
fi

CFLAGS="${CFLAGS} ${X_CFLAGS}"
LIBS="${LIBS} ${X_LIBS} ${X_PRE_LIBS} ${X_EXTRA_LIBS}"

PKG_CHECK_MODULES(GLIB, glib-2.0,,AC_MSG_ERROR([glib is required]))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
			
# Checks for libraries.
AC_CHECK_LIB([pthread],  [pthread_create],,            [AC_MSG_ERROR([libpthread not found])])
AC_CHECK_LIB([X11],      [XCreateWindow],,             [AC_MSG_ERROR([libX11 not found])])
AC_CHECK_LIB([Xxf86vm],  [XF86VidModeQueryExtension],, [AC_MSG_ERROR([libXxf86vm not found])])
AC_CHECK_LIB([Xxf86dga], [XF86DGAQueryExtension],,     [AC_MSG_ERROR([libXxf86dga not found])])
AC_CHECK_LIB([GL],       [glGetError],,                [AC_MSG_ERROR([libGL not found])])
AC_CHECK_LIB([glut],     [gluErrorString],,            [AC_MSG_ERROR([libglut not found])])
#AC_CHECK_LIB([dl], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h GL/gl.h GL/glx.h GL/glut.h],, [AC_MSG_ERROR([Required header file missing])])
AC_CHECK_HEADERS([GL/glATI.h GL/glxATI.h],,,
[#if HAVE_GL_GL_H
#include <GL/gl.h>
#endif

#if HAVE_GL_GLX_H
#include <GL/glx.h>
#endif
])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_C_BIGENDIAN

# Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([setenv strdup])

#AC_CONFIG_FILES(
AC_OUTPUT([Makefile])
