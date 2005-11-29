
PATCH		= linux-2.6-arp2.patch

all:		$(PATCH)

clean:
	$(RM) $(PATCH)

$(PATCH):
	diff -ruNw fc-current arp2-current > $@
