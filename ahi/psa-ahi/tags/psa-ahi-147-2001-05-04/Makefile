
CPPFLAGS	=
CFLAGS		= -O2 -W -Wall -Wno-unused -m68000
LDFLAGS		= -noixemul

AHIOUTFLAGS	= -DINFONAME=\"AHI\ S\"  -DPORTNAME=\"AHIOut\"
AHIINOUTFLAGS	= -DINFONAME=\"AHI\ HD\" -DPORTNAME=\"AHIInOut\"  \
		  -DENABLE_RECORD
AHIDUPLEXFLAGS	= -DINFONAME=\"AHI\ FD\" -DPORTNAME=\"AHIDuplex\" \
		  -DENABLE_RECORD -DENABLE_DUPLEX

.PHONY:		all clean distclean

all:		AHIOut AHIInOut AHIDuplex

clean:
	$(RM) *.o

distclean:
	$(RM) *.o AHIOut AHIInOut AHIDuplex


AHIInOut:	AHIInOut.o debug.o
	$(CC) $(LDFLAGS) $^ $(OUTPUT_OPTION)
	strip -s $@

AHIDuplex:	AHIDuplex.o debug.o
	$(CC) $(LDFLAGS) $^ $(OUTPUT_OPTION)
	strip -s $@

AHIOut:		AHIOut.o debug.o
	$(CC) $(LDFLAGS) $^ $(OUTPUT_OPTION)
	strip -s $@

AHIOut.o:	AHIInOut.c DriverTypes.h CompilerSpecific.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(AHIOUTFLAGS) -c $< $(OUTPUT_OPTION)

AHIInOut.o:	AHIInOut.c DriverTypes.h CompilerSpecific.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(AHIINOUTFLAGS) -c $< $(OUTPUT_OPTION)

AHIDuplex.o:	AHIInOut.c DriverTypes.h CompilerSpecific.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(AHIDUPLEXFLAGS) -c $< $(OUTPUT_OPTION)

debug.o:	debug.s
	basm -uo+ $< -o$@
