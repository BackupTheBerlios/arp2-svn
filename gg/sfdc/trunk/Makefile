
VERSION		= 1.0
DATE		= 2003-07-22

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
	sed "s/SFDC_VERSION/$(VERSION)/g" $^ | sed "s/SFDC_DATE/$(DATE)/g" >$@
	chmod +x $@

$(MAN):	$(EXECUTABLE)
	pod2man $^ -o $@

$(TXT):	$(EXECUTABLE)
	pod2text -c $^ -o $@
