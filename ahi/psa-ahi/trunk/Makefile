
CPPFLAGS	=
CFLAGS		= -O2 -W -Wall
LDFLAGS		= -noixemul

all:	AHIInOut

clean:
	$(RM) *.o AHIInOut

AHIInOut:	AHIInOut.o debug.o
	$(CC) $(LDFLAGS) $^ $(OUTPUT_OPTION)
	strip -s $@


debug.o:	debug.s
	basm -uo+ $< -o$@
