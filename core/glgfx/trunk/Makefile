
CPPFLAGS	= -DDEBUG -W -Wall
CFLAGS		= -O2 -pthread $(shell pkg-config glib-2.0 --cflags)
LDLIBS		= -L/usr/X11R6/lib -lX11 -lXxf86vm -lXext -ldl -lGL -lglut $(shell pkg-config glib-2.0 --libs)

GLGFX_OBJECTS	= glgfx.o glgfx_monitor.o glgfx_view.o glgfx_viewport.o glgfx_bitmap.o
GLGFX_TARGET	= glgfx

OBJECTS		= $(GLGFX_OBJECTS)
TARGETS		= $(GLGFX_TARGET)

all:		$(TARGETS)

clean:
	$(RM) $(TARGETS) $(OBJECTS) Makefile.dep

Makefile.dep:	$(GLGFX_OBJECTS:%.o=%.c)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -MM $^ > $@

include Makefile.dep

$(GLGFX_TARGET):	$(GLGFX_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION)

#glopen.o:	glfuncs.h
#
#glfuncs.h: /usr/include/GL/gl.h /usr/include/GL/glx.h
#	grep 'WINGDIAPI.*APIENTRY' /usr/include/GL/gl.h | sed 's/.*APIENTRY */GLFUNC(/' | sed 's/ .*/)/' > $@
#	grep 'glX' /usr/include/GL/glx.h | sed 's/.*glX/GLFUNC(glX/' | sed 's/.*glX/GLFUNC(glX/' | sed 's/\(glX[A-Za-z0-9_]*\).*/\1)/' >> $@

