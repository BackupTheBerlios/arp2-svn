
CPPFLAGS	=
CFLAGS		= -O2 -W -Wall
LDFLAGS		= -noixemul

all:	AHIInOut

clean:
	$(RM) *.o AHIInOut

AHIInOut:	AHIInOut.o
	$(CC) $(LDFLAGS) $^ $(OUTPUT_OPTION)

