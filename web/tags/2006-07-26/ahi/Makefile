
prefix	= m68k-amigaos

usrdocs	= COPYING COPYING.LIB COPYING.DRIVERS ChangeLog		\
	  User/Docs/ahiusr.html					\
	  User/Docs/ahiusr.pdf					\
	  User/Docs/ahiusr.ps

devdocs	= Developer/Docs/ahi.html				\
	  Developer/Docs/ahi_sub.html				\
	  Developer/Docs/ahidev.html				\
	  Developer/Docs/ahidev.pdf				\
	  Developer/Docs/ahidev.ps

docs		= $(notdir $(usrdocs) $(devdocs))

docfiles	= $(addprefix docs/, $(docs))
lhadevfiles	= $(addprefix $(prefix)-ahi/, $(devdocs))
lhausrfiles	= $(addprefix $(prefix)-ahi/, $(usrdocs))

help:
	@echo "Usage: make <version>"

all:	$(docfiles) index.html

clean:
	$(RM) $(docfiles) index.html


$(docfiles):	system/$(prefix)-ahidev-$(VER).lha		\
		system/$(prefix)-ahiusr-$(VER).lha
	cd docs && lha xif ../system/$(prefix)-ahidev-$(VER).lha $(lhadevfiles)
	cd docs && lha xif ../system/$(prefix)-ahiusr-$(VER).lha $(lhausrfiles)
	touch $(docfiles)

%.html:		_%.html
	@$(RM) $@
	@sed "s/&AHI;/<span class='caps'>A<span class='lower'>HI<\/span><\/span>/g" < $< | \
	sed "s,@FTP@,ftp://ftp.berlios.de/pub/arp2/ahi,g" | \
	sed "s/@VERSION@/$(VER)/g" | \
	sed "s/@DATE@/`date '+%Y-%m-%d'`/" > $@
	@chmod -w $@

system/$(prefix)-ahidev-$(VER).lha system/$(prefix)-ahiusr-$(VER).lha:
	@echo "Version $(VER) does not exists!"; false

.DEFAULT:
	@$(MAKE) --no-print-directory VER=$@ all
