#
# m32r/Makefile
#

LDFLAGS		:=
OBJCOPYFLAGS	:= -O binary -R .note -R .comment -S
LDFLAGS_vmlinux	:= -e startup_32

CFLAGS += -pipe -fno-schedule-insns
CFLAGS_KERNEL += -mmodel=medium
CFLAGS_MODULE += -mmodel=large

ifdef CONFIG_CHIP_VDEC2
cflags-$(CONFIG_ISA_M32R2)	+= -DNO_FPU -Wa,-bitinst
aflags-$(CONFIG_ISA_M32R2)	+= -DNO_FPU -Wa,-bitinst
else
cflags-$(CONFIG_ISA_M32R2)	+= -DNO_FPU -m32r2
aflags-$(CONFIG_ISA_M32R2)	+= -DNO_FPU -m32r2
endif

cflags-$(CONFIG_ISA_M32R)	+= -DNO_FPU
aflags-$(CONFIG_ISA_M32R)	+= -DNO_FPU -Wa,-no-bitinst

CFLAGS += $(cflags-y)
AFLAGS += $(aflags-y)

CHECKFLAGS	+= -D__m32r__ -D__BIG_ENDIAN__=1

head-y	:= arch/m32r/kernel/head.o arch/m32r/kernel/init_task.o

LIBGCC	:= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

libs-y	+= arch/m32r/lib/ $(LIBGCC)
core-y	+= arch/m32r/kernel/	\
	   arch/m32r/mm/	\
	   arch/m32r/boot/

drivers-$(CONFIG_OPROFILE)	+= arch/m32r/oprofile/

boot := arch/m32r/boot

.PHONY: zImage

all: zImage

zImage: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

compressed: zImage

archclean:
	$(Q)$(MAKE) $(clean)=$(boot)

define archhelp
	echo  '* zImage          - Compressed kernel image (arch/$(ARCH)/boot/zImage)'
endef
