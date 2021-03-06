#
# Makefile for the linux kernel.
#

extra-y := head.o init_task.o vmlinux.lds

obj-y	:= process.o semaphore.o signal.o entry.o traps.o irq.o \
		ptrace.o time.o ioport.o ldt.o setup.o i8259.o sys_i386.o \
		pci-dma.o i386_ksyms.o i387.o dmi_scan.o bootflag.o \
		quirks.o i8237.o topology.o

obj-y				+= cpu/
obj-y				+= timers/
obj-y				+= acpi/
obj-$(CONFIG_X86_BIOS_REBOOT)	+= reboot.o
obj-$(CONFIG_MCA)		+= mca.o
obj-$(CONFIG_X86_MSR)		+= msr.o
obj-$(CONFIG_X86_CPUID)		+= cpuid.o
obj-$(CONFIG_MICROCODE)		+= microcode.o
obj-$(CONFIG_APM)		+= apm.o
obj-$(CONFIG_X86_SMP)		+= smp.o smpboot.o
obj-$(CONFIG_X86_TRAMPOLINE)	+= trampoline.o
obj-$(CONFIG_X86_MPPARSE)	+= mpparse.o
obj-$(CONFIG_X86_LOCAL_APIC)	+= apic.o nmi.o
obj-$(CONFIG_X86_IO_APIC)	+= io_apic.o
obj-$(CONFIG_X86_REBOOTFIXUPS)	+= reboot_fixups.o
obj-$(CONFIG_KEXEC)		+= machine_kexec.o relocate_kernel.o crash.o
obj-$(CONFIG_CRASH_DUMP)	+= crash_dump.o
obj-$(CONFIG_X86_NUMAQ)		+= numaq.o
obj-$(CONFIG_X86_SUMMIT_NUMA)	+= summit.o
obj-$(CONFIG_KPROBES)		+= kprobes.o
obj-$(CONFIG_MODULES)		+= module.o
obj-y				+= sysenter.o vsyscall.o
obj-$(CONFIG_ACPI_SRAT) 	+= srat.o
obj-$(CONFIG_HPET_TIMER) 	+= time_hpet.o
obj-$(CONFIG_EFI) 		+= efi.o efi_stub.o
obj-$(CONFIG_DOUBLEFAULT) 	+= doublefault.o
obj-$(CONFIG_VM86)		+= vm86.o
obj-$(CONFIG_EARLY_PRINTK)	+= early_printk.o
obj-$(CONFIG_SMP_ALTERNATIVES)	+= smpalts.o

EXTRA_AFLAGS   := -traditional

obj-$(CONFIG_SCx200)		+= scx200.o

ifdef CONFIG_XEN
vsyscall_note := vsyscall-note-xen.o
else
vsyscall_note := vsyscall-note.o
endif

VSYSCALL_TYPES-y			:= int80
VSYSCALL_TYPES-$(CONFIG_X86_SYSENTER)	+= sysenter
# vsyscall.o contains the vsyscall DSO images as __initdata.
# We must build both images before we can assemble it.
# Note: kbuild does not track this dependency due to usage of .incbin
$(obj)/vsyscall.o: $(foreach F,$(VSYSCALL_TYPES-y),$(obj)/vsyscall-$F.so)
targets += $(foreach F,$(VSYSCALL_TYPES-y),vsyscall-$F.o vsyscall-$F.so)
targets += $(vsyscall_note) vsyscall.lds

# The DSO images are built using a special linker script.
quiet_cmd_syscall = SYSCALL $@
      cmd_syscall = $(CC) -m elf_i386 -nostdlib $(SYSCFLAGS_$(@F)) \
		          -Wl,-T,$(filter-out FORCE,$^) -o $@

export CPPFLAGS_vsyscall.lds += -P -C -U$(ARCH)

vsyscall-flags = -shared -s -Wl,-soname=linux-gate.so.1
SYSCFLAGS_vsyscall-sysenter.so	= $(vsyscall-flags)
SYSCFLAGS_vsyscall-int80.so	= $(vsyscall-flags)

$(obj)/vsyscall-int80.so $(obj)/vsyscall-sysenter.so: \
$(obj)/vsyscall-%.so: $(src)/vsyscall.lds \
		      $(obj)/vsyscall-%.o $(obj)/$(vsyscall_note) FORCE
	$(call if_changed,syscall)

# We also create a special relocatable object that should mirror the symbol
# table and layout of the linked DSO.  With ld -R we can then refer to
# these symbols in the kernel code rather than hand-coded addresses.
extra-y += vsyscall-syms.o
$(obj)/built-in.o: $(obj)/vsyscall-syms.o
$(obj)/built-in.o: ld_flags += -R $(obj)/vsyscall-syms.o

SYSCFLAGS_vsyscall-syms.o = -r
$(obj)/vsyscall-syms.o: $(src)/vsyscall.lds \
			$(obj)/vsyscall-sysenter.o $(obj)/$(vsyscall_note) FORCE
	$(call if_changed,syscall)

ifdef CONFIG_XEN
include $(srctree)/scripts/Makefile.xen

obj-y += fixup.o
microcode-$(subst m,y,$(CONFIG_MICROCODE)) := microcode-xen.o
n-obj-xen := i8259.o timers/ reboot.o smpboot.o trampoline.o

obj-y := $(call filterxen, $(obj-y), $(n-obj-xen))
obj-y := $(call cherrypickxen, $(obj-y))
extra-y := $(call cherrypickxen, $(extra-y))
endif
