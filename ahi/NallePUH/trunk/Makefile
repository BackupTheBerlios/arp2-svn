
CFLAGS	= -m68000 -mnobitfield -fomit-frame-pointer -O1 \
	  -W -Wall -Wno-unused -g
LDFLAGS	= -noixemul

PHXASSFLAGS	= NOEXE QUIET SET "CATCOMP_ARRAY" \
		  INCPATH Include: 

TARGET	= NallePUH
OBJECTS	= Nalle.o PUH.o GUICAT.o

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<

all:	$(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): 	$(OBJECTS) GUI.o
	$(CC) $(LDFLAGS) $(OUTPUT_OPTION) $^

GUICAT.s:	GUI.cd
	CatComp $< asmfile $@ XDEF

Nalle.o:	Nalle.c PUH.h
PUH.o:		PUH.c PUH.h

GUICAT.o:	GUICAT.s
	PhxAss $(PHXASSFLAGS) $< TO $@
