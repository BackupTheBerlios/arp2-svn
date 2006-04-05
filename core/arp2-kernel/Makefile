
FC_PATCH		= linux-2.6-fedora.patch
ARP2_PATCH		= linux-2.6-arp2.patch

PATCHES			= $(FC_PATCH) $(ARP2_PATCH)
SPEC			= arp2-kernel-2.6.spec
CONFIGS			= $(wildcard *arp2.config)

all:
	@echo "\"$(MAKE) patch\" to create patches"

patch:	 	$(PATCHES)

install:	$(PATCHES)
	cp $(PATCHES) $(CONFIGS) /usr/src/redhat/SOURCES/
	cp $(SPEC) /usr/src/redhat/SPECS/

clean:
	$(RM) $(PATCHES)

$(ARP2_PATCH):
	-diff -ruNw fc-current arp2-current > $@

$(FC_PATCH):
	-diff -ruNw linux-current fc-current > $@
