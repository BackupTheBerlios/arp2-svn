# $Id: acinclude.m4,v 1.1 1999/07/07 20:07:24 karl Exp $
# Source file used by aclocal in generating aclocal.m4.

#serial 3

dnl GG-local: Do not short-circuit running of the AC_CHECK_HEADERS macros 
dnl below.  The problem is that if you configure with a cache file that
dnl says these headers exist, then the macro doesn't get run and the
dnl appropriate HAVE_<file> macro does not get set in config.h.  As an
dnl example, just configure this twice in a row and compare the config.h
dnl files for each configure run.  -fnf

AC_DEFUN(jm_CHECK_DECLARATION,
[
  AC_REQUIRE([AC_HEADER_STDC])dnl
  AC_CHECK_HEADERS(memory.h)
  AC_CHECK_HEADERS(string.h)
  AC_CHECK_HEADERS(strings.h)
  AC_CHECK_HEADERS(stdlib.h)
  AC_CHECK_HEADERS(unistd.h)
  AC_MSG_CHECKING([whether $1 is declared])
  AC_CACHE_VAL(jm_cv_func_decl_$1,
    [AC_TRY_COMPILE($2,
      [
#ifndef $1
char *(*pfn) = (char *(*)) $1
#endif
      ],
      eval "jm_cv_func_decl_$1=yes",
      eval "jm_cv_func_decl_$1=no")])

  if eval "test \"`echo '$jm_cv_func_decl_'$1`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$3], , :, [$3])
  else
    AC_MSG_RESULT(no)
    ifelse([$4], , , [$4
])dnl
  fi
])dnl

dnl jm_CHECK_DECLARATIONS(INCLUDES, FUNCTION... [, ACTION-IF-DECLARED
dnl                       [, ACTION-IF-NOT-DECLARED]])
AC_DEFUN(jm_CHECK_DECLARATIONS,
[
  for jm_func in $2
  do
    jm_CHECK_DECLARATION($jm_func, $1,
    [
      jm_tr_func=HAVE_DECL_`echo $jm_func | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ`
      AC_DEFINE_UNQUOTED($jm_tr_func) $3], $4)dnl
  done
])

#serial 1
# this is check-decl.m4 in sh-utils 1.16k/m4/check-decl.m4
# with a different function list.

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN(txi_CHECK_DECLS,
[
  headers='
#include <stdio.h>
#ifdef HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
'

  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put entries
    dnl for each of these symbols in the config.h.in.
    dnl Otherwise, I'd have to update acconfig.h every time I change
    dnl this list of functions.
    AC_DEFINE(HAVE_DECL_STRERROR, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRCASECMP, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRNCASECMP, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRCOLL, 1, [Define if this function is declared.])
  fi

  jm_CHECK_DECLARATIONS($headers, strerror strcasecmp strncasecmp strcoll)
])
