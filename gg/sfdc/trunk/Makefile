
MAIN		= main.pl
CLASSES		= $(wildcard [A-Z]*.pl)
PODS		= SFDCompile.pod
EXECUTABLE	= SFDCompile
MAN		= SFDCompile.1
TXT		= SFDCompile.txt

all:	$(EXECUTABLE) $(MAN) $(TXT)

clean:
	$(RM) $(EXECUTABLE) $(MAN) $(TXT)

$(EXECUTABLE):	$(MAIN) $(CLASSES) $(PODS)
	cat >$@ $^
	chmod +x $@

$(MAN):	$(PODS)
	pod2man $^ -o $@

$(TXT):	$(PODS)
	pod2text -c $^ -o $@
