
CPPFLAGS	= -DDEBUG -W -Wall
CFLAGS		= -O2 -pthread $(shell pkg-config glib-2.0 --cflags)
LDLIBS		= -L/usr/X11R6/lib -lX11 -lXxf86vm -lGL -lglut $(shell pkg-config glib-2.0 --libs)

GLGFX_OBJECTS	= glgfx.o glgfx_monitor.o glgfx_input.o glgfx_view.o glgfx_viewport.o glgfx_bitmap.o
GLGFX_TARGET	= libglgfx.a

TEST_OBJECTS	= test.o
TEST_TARGET	= test

OBJECTS		= $(TEST_OBJECT) $(GLGFX_OBJECTS)
TARGETS		= $(TEST_TARGET) $(GLGFX_TARGET)

all:		$(TARGETS)

clean:
	$(RM) $(TARGETS) $(OBJECTS) Makefile.dep

$(GLGFX_TARGET):	$(GLGFX_OBJECTS)
	$(AR) cru $@ $^

$(TEST_TARGET):	$(TEST_OBJECTS) $(GLGFX_TARGET)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION)


# Remake automade files

LIBTOOL_DEPS = ./ltmain.sh
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

Makefile.dep:	$(GLGFX_OBJECTS:%.o=%.c)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -MM $^ > $@

include Makefile.dep
