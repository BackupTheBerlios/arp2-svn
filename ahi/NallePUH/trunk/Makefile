
CFLAGS	= -m68000 -mnobitfield -fomit-frame-pointer -O1 \
	  -W -Wall -Wno-unused -g
LDFLAGS	= -noixemul

PHXASSFLAGS	= NOEXE QUIET SET "CATCOMP_ARRAY" \
		  INCPATH Include: 

TARGET	= NallePUH
OBJECTS	= Nalle.o PUH.o NallePUHstr.o

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<

all:	$(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): 	$(OBJECTS) NallePUH.o
	$(CC) $(LDFLAGS) $(OUTPUT_OPTION) $^

NallePUHstr.s:	NallePUH.cd
	CatComp $< asmfile $@ XDEF

Nalle.o:	Nalle.c PUH.h NallePUH.h
PUH.o:		PUH.c PUH.h

NallePUHstr.o:	NallePUHstr.s
	PhxAss $(PHXASSFLAGS) $< TO $@
