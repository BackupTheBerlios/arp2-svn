## Process this file with automake to produce Makefile.in


include_HEADERS		= glgfx.h glgfx_monitor.h glgfx_input.h \
			  glgfx_view.h glgfx_viewport.h glgfx_bitmap.h

lib_LTLIBRARIES		= libglgfx.la

libglgfx_la_SOURCES	= glgfx.c glgfx_monitor.c glgfx_input.c \
			  glgfx_view.c glgfx_viewport.c glgfx_bitmap.c
libglgfx_la_CFLAGS	= @DEPS_CFLAGS@
libglgfx_la_LIBADD	= @DEPS_LIBS@

bin_PROGRAMS		= glgfx-test
glgfx_test_SOURCES	= test.c
glgfx_test_CFLAGS	= @DEPS_CFLAGS@
glgfx_test_LDADD	= libglgfx.la @DEPS_LIBS@
