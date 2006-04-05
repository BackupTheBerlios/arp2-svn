
SVN			= svn
SVK			= svk
SVN_REPO		= svn+ssh://svn.berlios.de/svnroot/repos/arp2

#svn diff svn+ssh://svn.berlios.de/svnroot/repos/arp2/core/vendor/kernel/linux/current/ svn+ssh://svn.berlios.de/svnroot/repos/arp2/core/vendor/kernel/fedora/current

FC_PATCH		= linux-2.6-fedora.patch
ARP2_PATCH		= linux-2.6-arp2.patch
ARP2_LOCAL_PATCH	= linux-2.6-arp2-local.patch

PATCHES			= $(FC_PATCH) $(ARP2_PATCH) $(ARP2_LOCAL_PATCH) 
SPEC			= arp2-kernel-2.6.spec
CONFIGS			= $(wildcard *arp2.config)

all:
	@echo "Use \"$(MAKE) patch\" to create patches from the Subversion repository at"
	@echo "\"$(SVN_REPO)\" and your local tree,"
	@echo "using \"$(SVN) diff\" and possibly \"$(SVK) diff\". It could take a while ..."

patch:		$(PATCHES)

install:	$(PATCHES)
	cp $(PATCHES) $(CONFIGS) /usr/src/redhat/SOURCES/
	cp $(SPEC) /usr/src/redhat/SPECS/

clean:
	$(RM) $(PATCHES)

$(ARP2_LOCAL_PATCH):
	-if [ -d current/.svn ]; then $(SVN) diff current > $@; else $(SVK) diff current > $@; fi

$(ARP2_PATCH):
	-$(SVN) diff $(SVN_REPO)/core/vendor/kernel/fedora/current/ \
	             $(SVN_REPO)/core/arp2-kernel/current/ > $@

$(FC_PATCH):
	-$(SVN) diff $(SVN_REPO)/core/vendor/kernel/linux/current/ \
	             $(SVN_REPO)/core/vendor/kernel/fedora/current/ > $@
