
CPPFLAGS	= -DDEBUG -W -Wall
CFLAGS		= -O2 -pthread
LDLIBS		= -L/usr/X11R6/lib -lX11 -lXxf86vm -lXext -ldl -lGL
LDFLAGS		= -pthread 

GLGFX_OBJECTS	= glgfx.o glgfx_monitor.o
GLGFX_TARGET	= glgfx

OBJECTS		= $(GLGFX_OBJECTS)
TARGETS		= $(GLGFX_TARGET)

all:		$(TARGETS)

clean:
	$(RM) $(TARGETS) $(OBJECTS)

$(GLGFX_TARGET):	$(GLGFX_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION)

#glopen.o:	glfuncs.h
#
#glfuncs.h: /usr/include/GL/gl.h /usr/include/GL/glx.h
#	grep 'WINGDIAPI.*APIENTRY' /usr/include/GL/gl.h | sed 's/.*APIENTRY */GLFUNC(/' | sed 's/ .*/)/' > $@
#	grep 'glX' /usr/include/GL/glx.h | sed 's/.*glX/GLFUNC(glX/' | sed 's/.*glX/GLFUNC(glX/' | sed 's/\(glX[A-Za-z0-9_]*\).*/\1)/' >> $@

