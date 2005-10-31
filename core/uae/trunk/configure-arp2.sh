$(dirname $0)/configure --prefix=/opt/arp2 \
	--disable-aga --enable-cdtv --disable-cd32 \
	--with-glgfx --with-glgfx-prefix=/opt/arp2 \
	--disable-audio \
	--disable-fdi --without-caps \
	--enable-cycle-exact-cpu --enable-compatible-cpu --enable-jit \
	--enable-natmem=0 --with-program-base=0x08000000 --enable-blomcall \
	--enable-autoconfig \
	--enable-scsi-device  --with-libscg-includedir=/usr/include/schily/ \
	--disable-enforcer --disable-action-replay \
	--disable-bsdsock --disable-bsdsock-new \
	--enable-threads --disable-ui 
