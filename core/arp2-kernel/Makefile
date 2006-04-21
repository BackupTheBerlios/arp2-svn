
SVK			= svk
SVK_ARP2_PATH		= //mirror/arp2/core/arp2-kernel/current
SVK_FC_PATH		= //mirror/arp2/core/vendor/kernel/fedora/current
SVK_LINUX_PATH		= //mirror/arp2/core/vendor/kernel/linux/current

#svn diff svn+ssh://svn.berlios.de/svnroot/repos/arp2/core/vendor/kernel/linux/current/ svn+ssh://svn.berlios.de/svnroot/repos/arp2/core/vendor/kernel/fedora/current

FC_PATCH		= linux-2.6-fedora.patch
ARP2_PATCH		= linux-2.6-arp2.patch
LOCAL_PATCH		= linux-2.6-local.patch

PATCHES			= $(FC_PATCH) $(ARP2_PATCH) $(LOCAL_PATCH) 
SPECS			= $(wildcard *.spec)
CONFIGS			= $(wildcard *arp2*.config)

all:
	@echo "Use \"$(MAKE) patch\" to create patches from the SVK repository at"
	@echo "$(SVK_LINUX_PATH),"
	@echo "$(SVK_FC_PATH),"
	@echo "$(SVK_ARP2_PATH),"
	@echo "and your local tree, using \"$(SVK) diff\". It could take a while ..."

patch:		$(PATCHES)

install:	$(PATCHES)
	cp $(PATCHES) $(CONFIGS) /usr/src/redhat/SOURCES/
	cp $(SPECS) /usr/src/redhat/SPECS/

clean:
	$(RM) $(PATCHES)

$(LOCAL_PATCH):
	-$(SVK) diff $(SVK_ARP2_PATH) current > $@

$(ARP2_PATCH):
	-$(SVK) diff $(SVK_FC_PATH) $(SVK_ARP2_PATH) > $@
#	-$(SVN) diff $(SVN_REPO)/core/vendor/kernel/fedora/current/ \
#	             $(SVN_REPO)/core/arp2-kernel/current/ > $@

$(FC_PATCH):
	-$(SVK) diff $(SVK_LINUX_PATH) $(SVK_FC_PATH) > $@
#	-$(SVN) diff $(SVN_REPO)/core/vendor/kernel/linux/current/ \
#	             $(SVN_REPO)/core/vendor/kernel/fedora/current/ > $@
