
CFLAGS	= -m68020-60 -mnobitfield -fomit-frame-pointer -O2 \
	  -W -Wall -Wno-unused
LDFLAGS	= -noixemul

PHXASSFLAGS	= NOEXE QUIET SET "CATCOMP_ARRAY" \
		  INCPATH Include: 

TARGET	= NallePUH
OBJECTS	= Nalle.o PUH.o NallePUHstr.o

%.o:	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<

all:	$(TARGET)


mostly-clean:
	$(RM) $(OBJECTS)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): 	$(OBJECTS) NallePUH.o pooh11.o
	$(CC) $(LDFLAGS) $(OUTPUT_OPTION) $^

NallePUHstr.s:	NallePUH.cd
	CatComp $< asmfile $@ XDEF

Nalle.o:	Nalle.c PUH.h NallePUH.h
PUH.o:		PUH.c PUH.h

NallePUHstr.o:	NallePUHstr.s
	PhxAss $(PHXASSFLAGS) $< TO $@
