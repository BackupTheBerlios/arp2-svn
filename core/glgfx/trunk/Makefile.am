## Process this file with automake to produce Makefile.in

AM_CPPFLAGS		= -I$(srcdir)/src
AM_CFLAGS		= -ggdb

include_HEADERS		= \
			src/glgfx.h \
			src/glgfx_bitmap.h \
			src/glgfx_context.h \
			src/glgfx_input.h \
			src/glgfx_monitor.h \
			src/glgfx_pixel.h \
			src/glgfx_sprite.h \
			src/glgfx_view.h \
			src/glgfx_viewport.h

lib_LTLIBRARIES		= libglgfx.la
noinst_PROGRAMS		= \
			tests/glgfx-test \
			tests/fpblit-test \
			tests/blit-test \
			tests/multithreaded \
			tests/gltex

libglgfx_la_SOURCES	= \
			src/glgfx.c \
			src/glgfx_bitmap.c \
			src/glgfx_context.c \
			src/glgfx_glxemul.c src/glgfx_glxemul.h \
			src/glgfx_glext.c src/glgfx_glext.h \
			src/glgfx_input.c \
			src/glgfx_monitor.c \
			src/glgfx_pixel.c \
			src/glgfx_sprite.c \
			src/glgfx_view.c \
			src/glgfx_viewport.c

libglgfx_la_CFLAGS	= @GLIB_CFLAGS@ -ggdb
libglgfx_la_CPPFLAGS	= -DDEBUG -I$(srcdir)/src
libglgfx_la_LIBADD	= @GLIB_LIBS@

tests_glgfx_test_LDADD	= libglgfx.la
tests_fpblit_test_LDADD	= libglgfx.la
tests_blit_test_LDADD	= libglgfx.la
tests_multithreaded_LDADD = libglgfx.la
