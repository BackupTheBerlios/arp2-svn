
CFLAGS	= -m68000 -mnobitfield -fomit-frame-pointer -O1 \
	  -W -Wall -Wno-unused -g
LDFLAGS	= -noixemul

TARGET	= NallePUH
OBJECTS	= Nalle.o PUH.o

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<

all:	$(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): 	$(OBJECTS)
	$(CC) $(LDFLAGS) $(OUTPUT_OPTION) $^

Nalle.o:	Nalle.c PUH.h
PUH.o:		PUH.c PUH.h
