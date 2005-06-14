## Process this file with automake to produce Makefile.in

include_HEADERS		= glgfx.h glgfx_monitor.h glgfx_input.h glgfx_pixel.h \
			  glgfx_view.h glgfx_viewport.h glgfx_bitmap.h
lib_LTLIBRARIES		= libglgfx.la
bin_PROGRAMS		= tests/glgfx-test tests/gltex

libglgfx_la_SOURCES	= glgfx.c glgfx_monitor.c glgfx_input.c glgfx_pixel.c \
			  glgfx_view.c glgfx_viewport.c glgfx_bitmap.c
libglgfx_la_CFLAGS	= @GLIB_CFLAGS@ -DDEBUG
libglgfx_la_LIBADD	= @GLIB_LIBS@

tests_glgfx_test_LDADD	= libglgfx.la
