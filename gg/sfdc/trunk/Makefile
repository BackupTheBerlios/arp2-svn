
MAIN		= main.pl
CLASSES		= $(wildcard [A-Z]*.pl)
PODS		= SFDCompile.pod
EXECUTABLE	= SFDCompile

all:	$(EXECUTABLE)

clean:
	$(RM) $(EXECUTABLE)

$(EXECUTABLE):	$(MAIN) $(CLASSES) $(PODS)
	cat >$@ $^
	chmod +x $@
