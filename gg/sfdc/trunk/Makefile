
MAIN		= main.pl
CLASSES		= $(wildcard [A-Z]*.pl)
PODS		= SFDCompile.pod
EXECUTABLE	= sfdc
MAN		= sfdc.1
TXT		= sfdc.txt

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
