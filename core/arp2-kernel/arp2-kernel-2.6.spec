Summary: The Linux kernel (the core of the Linux operating system)

# What parts do we want to build?  We must build at least one kernel.
# These are the kernels that are built IF the architecture allows it.

%define buildup 1
%define buildsmp 1
# Whether to apply the Xen patches, leave this enabled.
%define includexen 1
# Whether to build the Xen kernels, disable if you want.
%define buildxen 1
%define buildxenPAE 0
%define builddoc 0
%define buildkdump 1

# Versions of various parts

#
# Polite request for people who spin their own kernel rpms:
# please modify the "release" field in a way that identifies
# that the kernel isn't the stock distribution kernel, for example by
# adding some text to the end of the version number.
#
%define sublevel 16
%define kversion 2.6.%{sublevel}
%define rpmversion 2.6.%{sublevel}
%define rhbsys  %([ -r /etc/beehive-root -o -n "%{?__beehive_build}" ] && echo || echo .`whoami`)
%define release %(R="$Revision: 1.2080 $"; RR="${R##: }"; echo ${RR%%?})_FC5%{rhbsys}
%define signmodules 0
%define make_target bzImage
%define kernel_image x86

%define KVERREL %{PACKAGE_VERSION}-%{PACKAGE_RELEASE}

# groups of related archs
%define all_x86 i586 i686

# Override generic defaults with per-arch defaults 

%ifarch noarch
%define builddoc 1
%define buildup 0
%define buildsmp 0
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-*.config
%endif

# Xen and kdump only build on i686 and x86_64 ...
%ifnarch i686 x86_64
%define buildxen 0
%define buildkdump 0
%endif

# ... and XenPAE only on i686
%ifnarch i686
%define buildxenPAE 0
%endif

# Second, per-architecture exclusions (ifarch)

%ifarch i586
%define buildsmp 0
%endif

%ifarch %{all_x86}
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-i?86*.config
%define image_install_path boot
%define signmodules 1
%endif

%ifarch x86_64
%define buildsmp 0
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-x86_64*.config
%define image_install_path boot
%define signmodules 1
%endif

%ifarch ppc64
%define buildsmp 0
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-ppc64*.config
%define image_install_path boot
%define signmodules 1
%define make_target vmlinux
%define kernel_image vmlinux
%endif

%ifarch ppc64iseries
%define buildsmp 0
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-ppc64*.config
%define image_install_path boot
%define signmodules 1
%define make_target vmlinux
%define kernel_image vmlinux
%endif

%ifarch s390
%define buildsmp 0
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-s390*.config
%define image_install_path boot
%endif

%ifarch s390x
%define buildsmp 0
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-s390x.config
%define image_install_path boot
%endif

%ifarch sparc
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-sparc.config
%define buildsmp 0
%endif

%ifarch sparc64
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-sparc64*.config
%endif

%ifarch ppc
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-ppc*.config
%define image_install_path boot
%define make_target vmlinux
%define kernel_image vmlinux
%endif

%ifarch ia64
%define all_arch_configs $RPM_SOURCE_DIR/kernel-%{kversion}-ia64.config
%define buildsmp 0
%define image_install_path boot/efi/EFI/redhat
%define signmodules 1
%endif

#
# Three sets of minimum package version requirements in the form of Conflicts:
# to versions below the minimum
#

#
# First the general kernel 2.6 required versions as per
# Documentation/Changes
#
%define kernel_dot_org_conflicts  ppp < 2.4.3-3, isdn4k-utils < 3.2-32, nfs-utils < 1.0.7-12, e2fsprogs < 1.37-4, util-linux < 2.12, jfsutils < 1.1.7-2, reiserfs-utils < 3.6.19-2, xfsprogs < 2.6.13-4, procps < 3.2.5-6.3, oprofile < 0.9.1-2

# 
# Then a series of requirements that are distribution specific, either 
# because we add patches for something, or the older versions have 
# problems with the newer kernel or lack certain things that make 
# integration in the distro harder than needed.
#
%define package_conflicts kudzu < 1.2.5, initscripts < 7.23, udev < 063-6, iptables < 1.3.2-1, ipw2200-firmware < 2.4, selinux-policy-targeted < 1.25.3-14

#
# The ld.so.conf.d file we install uses syntax older ldconfig's don't grok.
#
%define xen_conflicts glibc < 2.3.5-1, xen < 3.0.1

#
# Packages that need to be installed before the kernel is, because the %post
# scripts use them.
#
%define kernel_prereq  fileutils, module-init-tools, initscripts >= 8.11.1-1, mkinitrd >= 4.2.21-1

Name: kernel
Group: System Environment/Kernel
License: GPLv2
Version: %{rpmversion}
Release: %{release}
ExclusiveArch: noarch %{all_x86} x86_64 ppc ppc64 ia64
ExclusiveOS: Linux
Provides: kernel = %{version}
Provides: kernel-drm = 4.3.0
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}
Prereq: %{kernel_prereq}
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

#
# List the packages used during the kernel build
#
BuildPreReq: module-init-tools, patch >= 2.5.4, bash >= 2.03, sh-utils, tar
BuildPreReq: bzip2, findutils, gzip, m4, perl, make >= 3.78, gnupg, diffutils
BuildRequires: gcc >= 3.4.2, binutils >= 2.12, redhat-rpm-config
BuildConflicts: rhbuildsys(DiskFree) < 500Mb


Source0: ftp://ftp.kernel.org/pub/linux/kernel/v2.6/linux-%{kversion}.tar.bz2
Source1: xen-20060301.tar.bz2
Source2: Config.mk

Source10: COPYING.modules
Source11: genkey

Source20: kernel-%{kversion}-i586.config
Source21: kernel-%{kversion}-i686.config
Source22: kernel-%{kversion}-i686-smp.config
Source23: kernel-%{kversion}-x86_64.config
Source24: kernel-%{kversion}-ppc64.config
Source25: kernel-%{kversion}-ppc64iseries.config
Source26: kernel-%{kversion}-s390.config
Source27: kernel-%{kversion}-s390x.config
Source28: kernel-%{kversion}-ppc.config
Source29: kernel-%{kversion}-ppc-smp.config
Source30: kernel-%{kversion}-ia64.config
Source31: kernel-%{kversion}-i686-xen0.config
Source32: kernel-%{kversion}-i686-xenU.config
Source33: kernel-%{kversion}-i686-kdump.config
Source33: kernel-%{kversion}-x86_64-kdump.config
#Source34: kernel-%{kversion}-sparc.config
#Source35: kernel-%{kversion}-sparc64.config
#Source36: kernel-%{kversion}-sparc64-smp.config
Source37: kernel-%{kversion}-i686-xen0-PAE.config
Source38: kernel-%{kversion}-i686-xenU-PAE.config
Source39: kernel-%{kversion}-x86_64-xen0.config
Source40: kernel-%{kversion}-x86_64-xenU.config

#
# Patches 0 through 100 are meant for core subsystem upgrades
#
Patch1: patch-2.6.16.1.bz2

# Patches 100 through 500 are meant for architecture patches
Patch100: linux-2.6-bzimage.patch

# 200 - 299   x86(-64)

Patch200: linux-2.6-x86-tune-p4.patch
Patch201: linux-2.6-x86-vga-vidfail.patch
Patch202: linux-2.6-intel-cache-build.patch
Patch203: linux-2.6-x86_64-silence-up-apic-errors.patch
Patch204: linux-2.6-edid-check.patch
Patch205: linux-2.6-x86_64-smp-on-uphw-cpucount.patch
Patch206: linux-2.6-x86-hp-reboot.patch
Patch207: linux-2.6-x86-cpu_index-false.patch
Patch208: linux-2.6-x86_64-vdso-compile-m32.patch

# 300 - 399   ppc(64)
Patch301: linux-2.6-cell-numa-init.patch
Patch302: linux-2.6-cell-is-cbea.patch
Patch305: linux-2.6-cell-mambo-drivers.patch
Patch306: linux-2.6-hvc-console.patch
Patch310: linux-2.6-cell-spiderpic-no-devtree.patch
Patch313: linux-2.6-hvc-rtas-console.patch
Patch314: linux-2.6-ppc-rtas-check.patch
Patch317: linux-2.6-ppc-iseries-input-layer.patch

# 400 - 499   ia64
# 500 - 599   s390(x)
# 600 - 699   sparc(64)

# 690 - 699 xen patches already split in their system
Patch690: linux-2.6-xen-i386-mach-io-check-nmi.patch
Patch691: linux-2.6-xen-net-csum.patch
Patch692: linux-2.6-xen-pmd-shared.patch
Patch693: linux-2.6-xen-smp-alts.patch
# 700 - 799 Xen
Patch700: linux-2.6-xen.patch
Patch701: linux-2.6-xen-compile-fixes.patch
Patch702: linux-2.6-xen-no-tls-warn.patch
Patch703: linux-2.6-xen-move-vdso-fixmap.patch
Patch704: linux-2.6-xen-vsyscall_base.patch
Patch706: linux-2.6-xen_remap_vma_flags.patch
Patch709: linux-2.6-percpu-NR_CPUS-hotplug-fix.patch
Patch710: linux-2.6-xen-kva-mmap.patch
Patch711: linux-2.6-xen-disable_ati_timer_quirk.patch
Patch712: linux-2.6-xen-time-went-backwards.patch

#
# Patches 800 through 899 are reserved for bugfixes to the core system
# and patches related to how RPMs are build
#
Patch800: linux-2.6-build-nonintconfig.patch
Patch801: linux-2.6-build-userspace-headers-warning.patch

# Exec-shield.
Patch810: linux-2.6-execshield.patch
Patch811: linux-2.6-execshield-xen.patch
Patch812: linux-2.6-xen-vdso-note.patch
Patch813: linux-2.6-vdso-xen.patch

# Module signing infrastructure.
Patch900: linux-2.6-modsign-core.patch
Patch901: linux-2.6-modsign-crypto.patch
Patch902: linux-2.6-modsign-ksign.patch
Patch903: linux-2.6-modsign-mpilib.patch
Patch904: linux-2.6-modsign-script.patch
Patch905: linux-2.6-modsign-include.patch

# Tux http accelerator.
Patch910: linux-2.6-tux.patch

#
# Patches 1000 to 5000 are reserved for bugfixes to drivers and filesystems
#

Patch1011: linux-2.6-debug-slab-backtrace.patch
Patch1012: linux-2.6-debug-list_head.patch
Patch1013: linux-2.6-debug-taint-vm.patch
Patch1014: linux-2.6-debug-slab-leaks.patch
Patch1015: linux-2.6-debug-singlebiterror.patch
Patch1016: linux-2.6-debug-spinlock-taint.patch
Patch1017: linux-2.6-debug-spinlock-panic.patch
Patch1018: linux-2.6-debug-Wundef.patch
Patch1019: linux-2.6-debug-disable-builtins.patch
Patch1020: linux-2.6-debug-sleep-in-irq-warning.patch
Patch1021: linux-2.6-debug-reference-discarded-return-result.patch
Patch1022: linux-2.6-debug-panic-stackdump.patch
Patch1024: linux-2.6-debug-dual-line-backtrace.patch
Patch1025: linux-2.6-debug-sysfs-crash-debugging.patch
Patch1026: linux-2.6-debug-no-quiet.patch
Patch1027: linux-2.6-debug-slab-leak-detector.patch
Patch1028: linux-2.6-debug-oops-pause.patch
Patch1029: linux-2.6-debug-account-kmalloc.patch
Patch1030: linux-2.6-debug-latency-tracing.patch
Patch1031: linux-2.6-debug-periodic-slab-check.patch
Patch1032: linux-2.6-debug-boot-delay.patch
Patch1033: linux-2.6-debug-must_check.patch
Patch1034: linux-2.6-debug-pm-pci.patch

# Restrict /dev/mem usage.
Patch1050: linux-2.6-devmem.patch
Patch1051: linux-2.6-devmem-xen.patch

# Provide read only /dev/crash driver.
Patch1060: linux-2.6-crash-driver.patch
Patch1061: linux-2.6-crash-xen.patch

Patch1070: linux-2.6-sleepon.patch

# SCSI bits.
Patch1101: linux-2.6-scsi-advansys-enabler.patch
Patch1102: linux-2.6-scsi-advansys-pcitable.patch

# NFS bits.
Patch1200: linux-2.6-NFSD-non-null-getxattr.patch
Patch1201: linux-2.6-NFSD-ctlbits.patch
Patch1203: linux-2.6-NFSD-badness.patch
Patch1204: linux-2.6-NFSD-writes-shouldnt-clobber-utimes.patch

# NIC driver updates
Patch1301: linux-2.6-net-sundance-ip100A.patch
Patch1302: linux-2.6-net-wireless-features.patch
Patch1303: linux-2.6-net-ipw2200-hwcrypto.patch
Patch1304: linux-2.6-net-ipw2200-monitor.patch

# Squashfs
Patch1400: linux-2.6-squashfs.patch

# Netdump and Diskdump bits.
Patch1500: linux-2.6-crashdump-common.patch
Patch1501: linux-2.6-netdump.patch
Patch1502: linux-2.6-netconsole.patch
Patch1503: linux-2.6-diskdump.patch
Patch1504: linux-2.6-crashdump-reboot-exports.patch
Patch1505: linux-2.6-dump_smp_call_function.patch

# Misc bits.
Patch1600: linux-2.6-module_version.patch
Patch1610: linux-2.6-input-kill-stupid-messages.patch
Patch1620: linux-2.6-serial-tickle-nmi.patch
Patch1630: linux-2.6-radeon-backlight.patch
Patch1640: linux-2.6-ide-tune-locking.patch
Patch1641: linux-2.6-ide-cd-shutup.patch
Patch1650: linux-2.6-autofs-pathlookup.patch
Patch1660: linux-2.6-valid-ether-addr.patch
Patch1670: linux-2.6-softcursor-persistent-alloc.patch
Patch1680: linux-2.6-pwc-powerup-by-default.patch
Patch1690: linux-2.6-smsc-ircc2-pnp.patch
Patch1700: linux-2.6-w1-hush-debug.patch
Patch1710: linux-2.6-sched-up-migration-cost.patch
Patch1730: linux-2.6-signal-trampolines-unwind-info.patch
Patch1740: linux-2.6-softlockup-disable.patch
Patch1750: linux-2.6-drm-cripple-r300.patch
Patch1760: linux-2.6-suspend-slab-warnings.patch
Patch1770: linux-2.6-optimise-spinlock-debug.patch
Patch1780: linux-2.6-cpufreq-acpi-sticky.patch

# SELinux/audit patches.
Patch1800: linux-2.6-selinux-hush.patch
Patch1801: linux-2.6-selinux-mprotect-checks.patch
Patch1802: linux-2.6-selinux-disable-attributes-no-policy.patch
Patch1803: linux-2.6-selinux-selinuxfs-hard-link-count.patch
Patch1804: linux-2.6-audit-new-msg-types.patch

# Warn about usage of various obsolete functionality that may go away.
Patch1900: linux-2.6-obsolete-idescsi-warning.patch
Patch1901: linux-2.6-obsolete-oss-warning.patch

# no external module should use these symbols.
Patch1910: linux-2.6-unexport-symbols.patch

# VM bits.
Patch2001: linux-2.6-vm-silence-atomic-alloc-failures.patch
Patch2002: linux-2.6-vm-clear-unreclaimable.patch

# Tweak some defaults.
Patch2100: linux-2.6-defaults-max-symlinks.patch
Patch2101: linux-2.6-defaults-fat-utf8.patch
Patch2102: linux-2.6-defaults-enable-sata-atapi.patch
Patch2103: linux-2.6-defaults-firmware-loader-timeout.patch

# SATA Bits
Patch2200: linux-2.6-sata-promise-pata-ports.patch
Patch2201: linux-2.6-sata-silence-dumb-msg.patch

# ACPI bits
Patch2300: linux-2.6-acpi_os_acquire_object-gfp_kernel-called-with-irqs.patch
Patch2301: linux-2.6-acpi-ecdt-uid-hack.patch

# Broadcom wireless driver
Patch5000: linux-2.6-softmac-git.patch
Patch5001: linux-2.6-bcm43xx-git.patch
Patch5002: linux-2.6-bcm43xx-neuter.patch
Patch5003: linux-2.6-softmac-scan-channel.patch
Patch5004: linux-2.6-softmac-scan-dwell-time.patch
Patch5005: linux-2.6-bcm43xx-assoc-on-startup.patch
Patch5006: linux-2.6-softmac-default-rate.patch
Patch5007: linux-2.6-bcm43xx-set-chan-lockup.patch

#
# 10000 to 20000 is for stuff that has to come last due to the
# amount of drivers they touch. But only these should go here.
# Not patches you're too lazy for to put in the proper place.
#

Patch10000: linux-2.6-compile-fixes.patch

# Little obvious 1-2 liners that fix silly bugs.
# Do not add anything non-trivial here.
Patch10001: linux-2.6-random-patches.patch

# Xen hypervisor patches
Patch20000: xen-sched-sedf.patch
Patch20001: xen-9232_fix_vmx.patch
Patch20002: xen-9236_fix_vmx.patch


# END OF PATCH DEFINITIONS

BuildRoot: %{_tmppath}/kernel-%{KVERREL}-root

%ifarch x86_64
Obsoletes: kernel-smp
%endif

%description 
The kernel package contains the Linux kernel (vmlinuz), the core of any
Linux operating system.  The kernel handles the basic functions
of the operating system:  memory allocation, process allocation, device
input and output, etc.

%package devel
Summary: Development package for building kernel modules to match the kernel.
Group: System Environment/Kernel
AutoReqProv: no
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}
Prereq: /usr/bin/find

%description devel
This package provides kernel headers and makefiles sufficient to build modules
against the kernel package.


%package doc
Summary: Various documentation bits found in the kernel source.
Group: Documentation

%description doc
This package contains documentation files from the kernel
source. Various bits of information about the Linux kernel and the
device drivers shipped with it are documented in these files. 

You'll want to install this package if you need a reference to the
options that can be passed to Linux kernel modules at load time.


%package smp
Summary: The Linux kernel compiled for SMP machines.

Group: System Environment/Kernel
Provides: kernel = %{version}
Provides: kernel-drm = 4.3.0
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}smp
Prereq: %{kernel_prereq}
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
# upto and including kernel 2.4.9 rpms, the 4Gb+ kernel was called kernel-enterprise
# now that the smp kernel offers this capability, obsolete the old kernel
Obsoletes: kernel-enterprise < 2.4.10
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

%description smp
This package includes a SMP version of the Linux kernel. It is
required only on machines with two or more CPUs as well as machines with
hyperthreading technology.

Install the kernel-smp package if your machine uses two or more CPUs.

%package smp-devel
Summary: Development package for building kernel modules to match the SMP kernel.
Group: System Environment/Kernel
Provides: kernel-smp-devel-%{_target_cpu} = %{rpmversion}-%{release}
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}smp
Provides: kernel-devel = %{rpmversion}-%{release}smp
AutoReqProv: no
Prereq: /usr/sbin/hardlink, /usr/bin/find

%description smp-devel
This package provides kernel headers and makefiles sufficient to build modules
against the SMP kernel package.

%package xen0
Summary: The Linux kernel compiled for Xen guest0 VM operations

Group: System Environment/Kernel
Provides: kernel = %{version}
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}xen0
Prereq: %{kernel_prereq}
Requires: xen
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
Conflicts: %{xen_conflicts}
# the hypervisor kernel needs a newer mkinitrd than everything else right now
Conflicts: mkinitrd <= 4.2.0 
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

%description xen0
This package includes a version of the Linux kernel which
runs in Xen's guest0 VM and provides device services to
the unprivileged guests.

Install this package in your Xen guest0 environment.

%package xen0-devel
Summary: Development package for building kernel modules to match the kernel.
Group: System Environment/Kernel
AutoReqProv: no
Provides: kernel-xen0-devel-%{_target_cpu} = %{rpmversion}-%{release}
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}xen0
Provides: kernel-devel = %{rpmversion}-%{release}xen0
Prereq: /usr/sbin/hardlink, /usr/bin/find

%description xen0-devel
This package provides kernel headers and makefiles sufficient to build modules
against the kernel package.


%package xen0-PAE
Summary: The Linux kernel compiled for Xen guest0 VM operations with PAE support

Group: System Environment/Kernel
Provides: kernel = %{version}
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}xen0-PAE
Prereq: %{kernel_prereq}
Requires: xen
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
Conflicts: %{xen_conflicts}
# the xen0-PAE kernel needs a newer mkinitrd than everything else right now
Conflicts: mkinitrd <= 4.2.0 
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

%description xen0-PAE
This package includes a version of the Linux kernel which runs in
Xen's guest0 VM with PAE support and provides device services to the
unprivileged guests.

Install this package in your Xen guest0 environment.


%package xen0-PAE-devel
Summary: Development package for building kernel modules to match the kernel.
Group: System Environment/Kernel
AutoReqProv: no
Provides: kernel-xen0-PAE-devel-%{_target_cpu} = %{rpmversion}-%{release}
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}xen0-PAE
Provides: kernel-devel = %{rpmversion}-%{release}xen0-PAE
Prereq: /usr/sbin/hardlink, /usr/bin/find

%description xen0-PAE-devel
This package provides kernel headers and makefiles sufficient to build modules
against the kernel package.

%package xenU
Summary: The Linux kernel compiled for unprivileged Xen guest VMs

Group: System Environment/Kernel
Provides: kernel = %{version}
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}xenU
Prereq: %{kernel_prereq}
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
Conflicts: %{xen_conflicts}
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

%description xenU
This package includes a version of the Linux kernel which
runs in Xen unprivileged guest VMs.  This should be installed
both inside the unprivileged guest (for the modules) and in
the guest0 domain.

%package xenU-devel
Summary: Development package for building kernel modules to match the kernel.
Group: System Environment/Kernel
AutoReqProv: no
Provides: kernel-xenU-devel-%{_target_cpu} = %{rpmversion}-%{release}
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}xenU
Provides: kernel-devel = %{rpmversion}-%{release}xenU
Prereq: /usr/sbin/hardlink, /usr/bin/find

%description xenU-devel
This package provides kernel headers and makefiles sufficient to build modules
against the kernel package.

%package xenU-PAE
Summary: The Linux kernel compiled for unprivileged Xen guest VMs with PAE support

Group: System Environment/Kernel
Provides: kernel = %{version}
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}xenU-PAE
Prereq: %{kernel_prereq}
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
Conflicts: %{xen_conflicts}
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

%description xenU-PAE
This package includes a version of the Linux kernel which runs in Xen
unprivileged guest VMs with PAE support.  This should be installed
both inside the unprivileged guest (for the modules) and in the guest0
domain.

%package xenU-PAE-devel
Summary: Development package for building kernel modules to match the kernel.
Group: System Environment/Kernel
AutoReqProv: no
Provides: kernel-xenU-PAE-devel-%{_target_cpu} = %{rpmversion}-%{release}
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}xenU-PAE
Provides: kernel-devel = %{rpmversion}-%{release}xenU-PAE
Prereq: /usr/sbin/hardlink, /usr/bin/find

%description xenU-PAE-devel
This package provides kernel headers and makefiles sufficient to build modules
against the kernel package.

%package kdump
Summary: A minimal Linux kernel compiled for kernel crash dumps.

Group: System Environment/Kernel
Provides: kernel = %{version}
Provides: kernel-drm = 4.3.0
Provides: kernel-%{_target_cpu} = %{rpmversion}-%{release}kdump
Prereq: %{kernel_prereq}
Conflicts: %{kernel_dot_org_conflicts}
Conflicts: %{package_conflicts}
# We can't let RPM do the dependencies automatic because it'll then pick up
# a correct but undesirable perl dependency from the module headers which
# isn't required for the kernel proper to function
AutoReqProv: no

%description kdump
This package includes a kdump version of the Linux kernel. It is
required only on machines which will use the kexec-base kernel crash dump
mechanism.

%package kdump-devel
Summary: Development package for building kernel modules to match the kdump kernel.
Group: System Environment/Kernel
Provides: kernel-kdump-devel-%{_target_cpu} = %{rpmversion}-%{release}
Provides: kernel-devel-%{_target_cpu} = %{rpmversion}-%{release}kdump
Provides: kernel-devel = %{rpmversion}-%{release}kdump
AutoReqProv: no
Prereq: /usr/sbin/hardlink, /usr/bin/find

%description kdump-devel
This package provides kernel headers and makefiles sufficient to build modules
against the kdump kernel package.


%prep
if [ ! -d kernel-%{kversion}/vanilla ]; then
  # Ok, first time we do a make prep.
  rm -f pax_global_header
%setup -q -n %{name}-%{version} -c -a1
  cp %{SOURCE2} .
  mv linux-%{kversion} vanilla
  mv xen xen-vanilla
  cp %{SOURCE2} .
else
  # We already have a vanilla dir.
  cd kernel-%{kversion}
  if [ -d linux-%{kversion}.%{_target_cpu} ]; then
     mv linux-%{kversion}.%{_target_cpu} deleteme
     rm -rf deleteme &
  fi
  if [ -d xen ]; then
     mv xen deleteme2
     rm -rf deleteme2 &
  fi
fi
cp -rl vanilla linux-%{kversion}.%{_target_cpu}
cp -rl xen-vanilla xen

# Any necessary hypervisor patches go here
%patch20000 -p1
%patch20001 -p1
%patch20002 -p1

cd linux-%{kversion}.%{_target_cpu}

# Update to latest upstream.
%patch1 -p1

#
# Patches 10 through 100 are meant for core subsystem upgrades
#

#
# Patches to back out
#

#
# Architecture patches
#
%patch100 -p1

#
# x86(-64)
#
# Compile 686 kernels tuned for Pentium4.
%patch200 -p1
# add vidfail capability;
# without this patch specifying a framebuffer on the kernel prompt would
# make the boot stop if there's no supported framebuffer device; this is bad
# for the installer cd that wants to automatically fall back to textmode
# in that case
%patch201 -p1
# exitfunc called from initfunc.
%patch202 -p1
# Suppress APIC errors on UP x86-64.
%patch203 -p1
# Reboot thru bios on HP laptops.
%patch204 -p1
# Workaround BIOSes that don't list CPU0
%patch205 -p1
# Reboot through BIOS on HP systems,.
%patch206 -p1
# cpu_index >= NR_CPUS becomming always false.
%patch207 -p1
# Fix broken x86-64 32bit vDSO
%patch208 -p1

#
# ppc64
#
# Arnd says don't call cell_spumem_init() till he fixes it.
%patch301 -p1
# IBM will use 'IBM,CBEA' for future Cell systems
%patch302 -p1
# Support the IBM Mambo simulator; core as well as disk and network drivers.
%patch305 -p1
# Make HVC console generic; support simulator console device using it.
%patch306 -p1
# Hardcode PIC addresses for Cell spiderpic
%patch310 -p1
# RTAS console support
%patch313 -p1
# Check properly for successful RTAS instantiation
%patch314 -p1
# No input layer on iseries
%patch317 -p1

#
# Xen
#
%if %{includexen}
# Base Xen patch from linux-2.6-merge.hg
%patch690 -p1
# Conflict with non-xen kernels
#%patch691 -p1
%patch692 -p1
%patch693 -p1

#
# Apply the main xen patch...
#
%patch700 -p1 -b .p.xen
#
# ... and back out all the ia64-specific sections, as they currently prevent
# non-xen builds from working.
#
for f in `find arch/ia64/ include/asm-ia64/ include/xen/interface/arch-ia64.h* -type f -name "*.p.xen"` ; do \
    g=`dirname $f`/`basename $f .p.xen`; \
    mv "$f" "$g"; \
    if [ ! -s "$g" ] ; then rm -f "$g" ; fi; \
done
# Delete the rest of the backup files, they just confuse the build later
find -name "*.p.xen" | xargs rm -f
    
#
# Xen includes a patch which moves the vsyscall fixmap into a user-space VA,
# freeing user-space from reliance on an absolute fixmap area and so allowing
# the fixmap area to become dynamic.
#
# Execshield already does this, making the fixmap area invisible to the user
# and adding a new randomised vdso for it in user VA, so there's no point in
# having both: revert the Xen changeset so that execsheild applies cleanly.
#
%patch703 -p2 -R

%patch701 -p1
%patch702 -p1
%patch704 -p1
%patch706 -p1
%patch709 -p1
%patch710 -p2
%patch711 -p1
%patch712 -p1

%endif

#
# Patches 500 through 1000 are reserved for bugfixes to the core system
# and patches related to how RPMs are build
#


# This patch adds a "make nonint_oldconfig" which is non-interactive and
# also gives a list of missing options at the end. Useful for automated
# builds (as used in the buildsystem).
%patch800 -p1
# Warn if someone tries to build userspace using kernel headers
%patch801 -p1

# Exec shield 
%patch810 -p1

# Xen exec-shield bits
%if %{includexen}
%patch811 -p1
%patch812 -p1
#%patch813 -p1
%endif

#
# GPG signed kernel modules
#
%patch900 -p1
%patch901 -p1
%patch902 -p1
%patch903 -p1
%patch904 -p1
%patch905 -p1

# Tux
%patch910 -p1

#
# Patches 1000 to 5000 are reserved for bugfixes to drivers and filesystems
#


# Various low-impact patches to aid debugging.
%patch1011 -p1
%patch1012 -p1
%patch1013 -p1
%patch1014 -p1
%patch1015 -p1
%patch1016 -p1
%patch1017 -p1
%patch1018 -p1
%patch1019 -p1
%patch1020 -p1
%patch1021 -p1
%patch1022 -p1
%patch1024 -p1
%patch1025 -p1
# Disable the 'quiet' boot switch for better bug reports.
#%patch1026 -p1
# Slab leak detector.
#%patch1027 -p1
%patch1028 -p1
#%patch1029 -p1
#%patch1030 -p1
%patch1031 -p1
%patch1032 -p1
%patch1033 -p1
%patch1034 -p1

#
# Make /dev/mem a need-to-know function 
#
%patch1050 -p1
%if %{includexen}
#TODO! --sct @@@
#%patch1051 -p1
%endif

#
# /dev/crash driver for the crashdump analysis tool
#
%patch1060 -p1
%if %{includexen}
%patch1061 -p1
%endif

#
# Most^WAll users of sleep_on are broken; fix a bunch
#
%patch1070 -p1

#
# SCSI Bits.
#
# Enable Advansys driver
%patch1101 -p1
# Add a pci table to advansys driver.
%patch1102 -p1

#
# Various upstream NFS/NFSD fixes.
#
#%patch1200 -p1
# kNFSD: fixed '-p port' arg to rpc.nfsd and enables the defining proto versions and transports
%patch1201 -p1
# Fix badness.
%patch1203 -p1
# NFSD: writes should not clobber utimes() calls
%patch1204 -p1

# NIC driver fixes.
# New PCI ID for sundance driver.
%patch1301 -p1
# Goodies for wireless drivers to make NetworkManager work
%patch1302 -p1
# ipw2200 hwcrypto=0 by default to avoid firmware restarts
%patch1303 -p1
# add IPW2200_MONITOR config option
%patch1304 -p1

# Squashfs
%patch1400 -p1

# netdump bits
%patch1500 -p1
%patch1501 -p1
%patch1502 -p1
%patch1503 -p1
%patch1504 -p1
%patch1505 -p1

#
# Various SELinux fixes from 2.6.10rc
#

# Misc fixes
# Add missing MODULE_VERSION tags to some modules.
%patch1600 -p1
# The input layer spews crap no-one cares about.
%patch1610 -p1
# Tickle the NMI whilst doing serial writes.
%patch1620 -p1
# Radeon on thinkpad backlight power-management goodness.
%patch1630 -p1
# Fix IDE locking bug.
%patch1640 -p1
# Silence noisy CD drive spew
%patch1641 -p1
# autofs4 looks up wrong path element when ghosting is enabled
%patch1650 -p1
# 
%patch1660 -p1
# Use persistent allocation in softcursor
%patch1670 -p1
# Power up PWC driver by default.
%patch1680 -p1
# PNP support for smsc-ircc2
%patch1690 -p1
# Silence debug messages in w1
%patch1700 -p1
# Only print migration info on SMP
%patch1710 -p1
# Mark unwind info for signal trampolines in vDSOs
%patch1730 -p1
# Add a safety net to softlockup so that it doesn't prevent installs.
%patch1740 -p1
# Disable R300 and above DRI as it's unstable.
%patch1750 -p1
# Fix up kmalloc whilst atomic warning during resume.
%patch1760 -p1
# Speed up spinlock debug.
%patch1770 -p1
# Make acpi-cpufreq sticky.
%patch1780 -p1

# Silence some selinux messages.
%patch1800 -p1
# Fix the SELinux mprotect checks on executable mappings
%patch1801 -p1
# Disable setting of security attributes on new inodes when no policy is loaded
%patch1802 -p1
# Fix incorrect hardlink count in selinuxfs
%patch1803 -p1
# Add some more audit message types.
%patch1804 -p1

# Warn about obsolete functionality usage.
%patch1900 -p1
%patch1901 -p1
# Remove kernel-internal functionality that nothing external should use.
%patch1910 -p1

#
# VM related fixes.
#
# Silence GFP_ATOMIC failures.
%patch2001 -p1
# VM oom killer tweaks.
%patch2002 -p1

# Changes to upstream defaults.
# Bump up the number of recursive symlinks.
%patch2100 -p1
# Use UTF-8 by default on VFAT.
%patch2101 -p1
# Enable SATA ATAPI by default.
%patch2102 -p1
# Increase timeout on firmware loader.
%patch2103 -p1

# Enable PATA ports on Promise SATA.
%patch2200 -p1
# Silence silly SATA printk.
%patch2201 -p1

# Silence more ACPI debug spew from suspend.
%patch2300 -p1
# acpi-ecdt-uid-hack
%patch2301 -p1


#
# Patches 5000 to 6000 are reserved for new drivers that are about to
# be merged upstream
#

# Import softmac code from wireless-2.6 tree
%patch5000 -p1
# ... and bcm43xx driver too
%patch5001 -p1
# temporarily remove bcm43xx's MODULE_DEVICE_TABLE entry
%patch5002 -p1
# Go back to the original channel when we finish scanning.
%patch5003 -p1
# When scanning, spend only 20ms on each channel not 500ms.
%patch5004 -p1
# Attempt to associate when the link is brought up
%patch5005 -p1
# Default to 11Mbps not 54Mbps, since we don't back down automatically yet.
%patch5006 -p1
# bcm43xx dies if we attempt to set the channel while it's down.
%patch5007 -p1

#
# final stuff
#

#
# misc small stuff to make things compile or otherwise improve performance
#
#%patch10000 -p1

# Small 1-2 liners fixing silly bugs that get pushed upstream quickly.
%patch10001 -p1


# END OF PATCH APPLICATIONS

cp %{SOURCE10} Documentation/

mkdir configs

cp -f %{all_arch_configs} .


# now run oldconfig over all the config files
for i in *.config
do 
  mv $i .config 
  Arch=`head -1 .config | cut -b 3-`
%if %{includexen}
  make ARCH=$Arch nonint_oldconfig > /dev/null
%else
  if [ "$Arch" != "xen" ]; then
    make ARCH=$Arch nonint_oldconfig > /dev/null
  fi
%endif
  echo "# $Arch" > configs/$i
  cat .config >> configs/$i 
done

# make sure the kernel has the sublevel we know it has. This looks weird
# but for -pre and -rc versions we need it since we only want to use
# the higher version when the final kernel is released.
perl -p -i -e "s/^SUBLEVEL.*/SUBLEVEL = %{sublevel}/" Makefile
perl -p -i -e "s/^EXTRAVERSION.*/EXTRAVERSION = -prep/" Makefile

# get rid of unwanted files resulting from patch fuzz
find . -name "*.orig" -o -name "*~" -exec rm -f {} \; >/dev/null &

###
### build
###
%build
#
# Create gpg keys for signing the modules
#

gpg --homedir . --batch --gen-key %{SOURCE11}  
gpg --homedir . --export --keyring ./kernel.pub Red > extract.pub 
make linux-%{kversion}.%{_target_cpu}/scripts/bin2c
linux-%{kversion}.%{_target_cpu}/scripts/bin2c ksign_def_public_key __initdata < extract.pub > linux-%{kversion}.%{_target_cpu}/crypto/signature/key.h

BuildKernel() {
    MakeTarget=$1
    KernelImage=$2
    Flavour=$3

    # Pick the right config file for the kernel we're building
    if [ -n "$Flavour" ] ; then
      Config=kernel-%{kversion}-%{_target_cpu}-$Flavour.config
      DevelDir=/usr/src/kernels/%{KVERREL}-$Flavour-%{_target_cpu}
      DevelLink=/usr/src/kernels/%{KVERREL}$Flavour-%{_target_cpu}
    else
      Config=kernel-%{kversion}-%{_target_cpu}.config
      DevelDir=/usr/src/kernels/%{KVERREL}-%{_target_cpu}
      DevelLink=
    fi

    KernelVer=%{version}-%{release}$Flavour
    echo BUILDING A KERNEL FOR $Flavour %{_target_cpu}...

    # make sure EXTRAVERSION says what we want it to say
    perl -p -i -e "s/^EXTRAVERSION.*/EXTRAVERSION = -%{release}$Flavour/" Makefile

    # and now to start the build process

    make -s mrproper
    cp configs/$Config .config

    Arch=`head -1 .config | cut -b 3-`
    echo USING ARCH=$Arch

    if [ "$KernelImage" == "x86" ]; then
       KernelImage=arch/$Arch/boot/bzImage
    fi

    make -s ARCH=$Arch nonint_oldconfig > /dev/null
    make -s ARCH=$Arch %{?_smp_mflags} $MakeTarget
    make -s ARCH=$Arch %{?_smp_mflags} modules || exit 1

    # Start installing the results

%if "%{_enable_debug_packages}" == "1"
    mkdir -p $RPM_BUILD_ROOT/usr/lib/debug/boot
%endif
    mkdir -p $RPM_BUILD_ROOT/%{image_install_path}
    install -m 644 .config $RPM_BUILD_ROOT/boot/config-$KernelVer
    install -m 644 System.map $RPM_BUILD_ROOT/boot/System.map-$KernelVer
    cp $KernelImage $RPM_BUILD_ROOT/%{image_install_path}/vmlinuz-$KernelVer
    if [ -f arch/$Arch/boot/zImage.stub ]; then
      cp arch/$Arch/boot/zImage.stub $RPM_BUILD_ROOT/%{image_install_path}/zImage.stub-$KernelVer || :
    fi

    if [ "$Flavour" == "kdump" ]; then
        cp vmlinux $RPM_BUILD_ROOT/%{image_install_path}/vmlinux-$KernelVer
        rm -f $RPM_BUILD_ROOT/%{image_install_path}/vmlinuz-$KernelVer
    fi

    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer
    make -s ARCH=$Arch INSTALL_MOD_PATH=$RPM_BUILD_ROOT modules_install KERNELRELEASE=$KernelVer
 
    # And save the headers/makefiles etc for building modules against
    #
    # This all looks scary, but the end result is supposed to be:
    # * all arch relevant include/ files
    # * all Makefile/Kconfig files
    # * all script/ files 

    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/source
    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    (cd $RPM_BUILD_ROOT/lib/modules/$KernelVer ; ln -s build source)
    # dirs for additional modules per module-init-tools, kbuild/modules.txt
    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/extra
    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/updates
    # first copy everything
    cp --parents `find  -type f -name "Makefile*" -o -name "Kconfig*"` $RPM_BUILD_ROOT/lib/modules/$KernelVer/build 
    cp Module.symvers $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    # then drop all but the needed Makefiles/Kconfig files
    rm -rf $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/Documentation
    rm -rf $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/scripts
    rm -rf $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    cp arch/%{_arch}/kernel/asm-offsets.s $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/%{_arch}/kernel || :
    cp .config $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    cp .kernelrelease $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/
    cp -a scripts $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    if [ -d arch/%{_arch}/scripts ]; then
      cp -a arch/%{_arch}/scripts $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/%{_arch} || :
    fi
    if [ -f arch/%{_arch}/*lds ]; then
      cp -a arch/%{_arch}/*lds $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/%{_arch}/ || :
    fi
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/scripts/*.o
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/scripts/*/*.o
    mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    cd include
    cp -a acpi config linux math-emu media net pcmcia rxrpc scsi sound video asm asm-generic $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    cp -a `readlink asm` $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    if [ "$Arch" = "x86_64" ]; then
      cp -a asm-i386 $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
    fi
    # While arch/powerpc/include/asm is still a symlink to the old
    # include/asm-ppc{64,} directory, include that in kernel-devel too.
    if [ "$Arch" = "powerpc" -a -r ../arch/powerpc/include/asm ]; then
      cp -a `readlink ../arch/powerpc/include/asm` $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
      mkdir -p $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/$Arch/include
      pushd $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/arch/$Arch/include
      ln -sf ../../../include/asm-ppc* asm
      popd
    fi
%if %{buildxen}
    cp -a xen $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include
%endif

    # Make sure the Makefile and version.h have a matching timestamp so that
    # external modules can be built
    touch -r $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/Makefile $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include/linux/version.h
    touch -r $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/.config $RPM_BUILD_ROOT/lib/modules/$KernelVer/build/include/linux/autoconf.h
    cd .. 

    #
    # save the vmlinux file for kernel debugging into the kernel-debuginfo rpm
    #
%if "%{_enable_debug_packages}" == "1"
    mkdir -p $RPM_BUILD_ROOT/usr/lib/debug/lib/modules/$KernelVer
    cp vmlinux $RPM_BUILD_ROOT/usr/lib/debug/lib/modules/$KernelVer
%endif

    find $RPM_BUILD_ROOT/lib/modules/$KernelVer -name "*.ko" -type f >modnames

    # gpg sign the modules
%if %{signmodules}
    gcc -o scripts/modsign/mod-extract scripts/modsign/mod-extract.c -Wall
    KEYFLAGS="--no-default-keyring --homedir .." 
    KEYFLAGS="$KEYFLAGS --secret-keyring ../kernel.sec" 
    KEYFLAGS="$KEYFLAGS --keyring ../kernel.pub" 
    export KEYFLAGS 

    for i in `cat modnames`
    do
      sh ./scripts/modsign/modsign.sh $i Red
      mv -f $i.signed $i
    done
    unset KEYFLAGS
%endif

    # mark modules executable so that strip-to-file can strip them
    cat modnames | xargs chmod u+x

    # detect missing or incorrect license tags
    for i in `cat modnames`
    do
      echo -n "$i " 
      /sbin/modinfo -l $i >> modinfo
    done
    cat modinfo |\
      grep -v "^GPL" |
      grep -v "^Dual BSD/GPL" |\
      grep -v "^Dual MPL/GPL" |\
      grep -v "^GPL and additional rights" |\
      grep -v "^GPL v2" && exit 1 
    rm -f modinfo
    rm -f modnames
    # remove files that will be auto generated by depmod at rpm -i time
    rm -f $RPM_BUILD_ROOT/lib/modules/$KernelVer/modules.*

    # Move the devel headers out of the root file system
    mkdir -p $RPM_BUILD_ROOT/usr/src/kernels
    mv $RPM_BUILD_ROOT/lib/modules/$KernelVer/build $RPM_BUILD_ROOT/$DevelDir
    ln -sf ../../..$DevelDir $RPM_BUILD_ROOT/lib/modules/$KernelVer/build
    [ -z "$DevelLink" ] || ln -sf `basename $DevelDir` $RPM_BUILD_ROOT/$DevelLink
}

###
# DO it...
###

# prepare directories
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/boot

%if %{buildxen}
  cd xen
  mkdir -p $RPM_BUILD_ROOT/%{image_install_path}
%if %{buildxenPAE}
  make debug=y verbose=y crash_debug=y pae=y
  install -m 644 xen.gz $RPM_BUILD_ROOT/boot/xen.gz-%{KVERREL}-PAE
  install -m 755 xen-syms $RPM_BUILD_ROOT/boot/xen-syms-%{KVERREL}-PAE
  make clean
%endif
  make debug=y verbose=y crash_debug=y
  install -m 644 xen.gz $RPM_BUILD_ROOT/boot/xen.gz-%{KVERREL}
  install -m 755 xen-syms $RPM_BUILD_ROOT/boot/xen-syms-%{KVERREL}
  cd ..
%endif

cd linux-%{kversion}.%{_target_cpu}

%if %{buildup}
BuildKernel %make_target %kernel_image
%endif

%if %{buildsmp}
BuildKernel %make_target %kernel_image smp
%endif

%if %{buildxenPAE}
BuildKernel vmlinuz vmlinuz xen0-PAE
BuildKernel vmlinuz vmlinuz xenU-PAE
%endif

%if %{buildxen}
BuildKernel vmlinuz vmlinuz xen0
BuildKernel vmlinuz vmlinuz xenU
%endif

%if %{buildkdump}
BuildKernel %make_target %kernel_image kdump
%endif

###
### install
###

%install

cd linux-%{kversion}.%{_target_cpu}

%if %{buildxen}
mkdir -p $RPM_BUILD_ROOT/etc/ld.so.conf.d
rm -f $RPM_BUILD_ROOT/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf
cat > $RPM_BUILD_ROOT/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf <<\EOF
# This directive teaches ldconfig to search in nosegneg subdirectories
# and cache the DSOs there with extra bit 0 set in their hwcap match
# fields.  In Xen guest kernels, the vDSO tells the dynamic linker to
# search in nosegneg subdirectories and to match this extra hwcap bit
# in the ld.so.cache file.
hwcap 0 nosegneg
EOF
chmod 444 $RPM_BUILD_ROOT/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf
%endif

%if %{builddoc}
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/kernel-doc-%{kversion}/Documentation

# sometimes non-world-readable files sneak into the kernel source tree
chmod -R a+r *
# copy the source over
tar cf - Documentation | tar xf - -C $RPM_BUILD_ROOT/usr/share/doc/kernel-doc-%{kversion}
%endif

###
### clean
###

%clean
rm -rf $RPM_BUILD_ROOT

###
### scripts
###

# load the loop module for upgrades...in case the old modules get removed we have
# loopback in the kernel so that mkinitrd will work.
%pre 
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
exit 0

%pre smp
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
exit 0

%post 
if [ `uname -i` == "x86_64" ]; then
  if [ -f /etc/sysconfig/kernel ]; then
    /bin/sed -i -e 's/^DEFAULTKERNEL=kernel-smp$/DEFAULTKERNEL=kernel/' /etc/sysconfig/kernel
  fi
fi
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}
/sbin/new-kernel-pkg --package kernel --mkinitrd --depmod --install %{KVERREL}

%post devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%post smp
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}smp
/sbin/new-kernel-pkg --package kernel-smp --mkinitrd --depmod --install %{KVERREL}smp

%post smp-devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-smp-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%post xen0
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}-xen0
/sbin/new-kernel-pkg --package kernel-xen0 --mkinitrd --depmod --install --multiboot=/boot/xen.gz-%{KVERREL} %{KVERREL}xen0
[ ! -x /sbin/ldconfig ] || /sbin/ldconfig -X

%post xen0-devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-xen0-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%post xenU
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}-xenU
/sbin/new-kernel-pkg --package kernel-xenU --mkinitrd --depmod --install %{KVERREL}xenU
[ ! -x /sbin/ldconfig ] || /sbin/ldconfig -X

%post xenU-devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-xenU-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%post xen0-PAE
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}-xen0-PAE
/sbin/new-kernel-pkg --package kernel-xen0-PAE --mkinitrd --depmod --install --multiboot=/boot/xen.gz-%{KVERREL}-PAE %{KVERREL}xen0-PAE
[ ! -x /sbin/ldconfig ] || /sbin/ldconfig -X

%post xen0-PAE-devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-xen0-PAE-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%post xenU-PAE
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}-xenU-PAE
/sbin/new-kernel-pkg --package kernel-xenU-PAE --mkinitrd --depmod --install %{KVERREL}xenU-PAE
[ ! -x /sbin/ldconfig ] || /sbin/ldconfig -X

%post xenU-PAE-devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-xenU-PAE-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%post kdump
[ ! -x /usr/sbin/module_upgrade ] || /usr/sbin/module_upgrade %{rpmversion}-%{release}-kdump
/sbin/new-kernel-pkg --package kernel-kdump --mkinitrd --depmod --install %{KVERREL}kdump

%post kdump-devel
[ -f /etc/sysconfig/kernel ] && . /etc/sysconfig/kernel
if [ "$HARDLINK" != "no" -a -x /usr/sbin/hardlink ] ; then
  pushd /usr/src/kernels/%{KVERREL}-kdump-%{_target_cpu} > /dev/null
  /usr/bin/find . -type f | while read f; do hardlink -c /usr/src/kernels/*FC*/$f $f ; done
  popd > /dev/null
fi

%preun 
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
/sbin/new-kernel-pkg --rminitrd --rmmoddep --remove %{KVERREL}

%preun smp
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
/sbin/new-kernel-pkg --rminitrd --rmmoddep --remove %{KVERREL}smp

%preun xen0
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
/sbin/new-kernel-pkg --rminitrd --rmmoddep --remove %{KVERREL}xen0

%preun xenU
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
/sbin/new-kernel-pkg --rmmoddep --remove %{KVERREL}xenU

%preun xen0-PAE
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
/sbin/new-kernel-pkg --rminitrd --rmmoddep --remove %{KVERREL}xen0-PAE

%preun xenU-PAE
/sbin/modprobe loop 2> /dev/null > /dev/null  || :
/sbin/new-kernel-pkg --rmmoddep --remove %{KVERREL}xenU-PAE


###
### file lists
###

%if %{buildup}
%files 
%defattr(-,root,root)
/%{image_install_path}/vmlinuz-%{KVERREL}
/boot/System.map-%{KVERREL}
/boot/config-%{KVERREL}
%dir /lib/modules/%{KVERREL}
/lib/modules/%{KVERREL}/kernel
/lib/modules/%{KVERREL}/build
/lib/modules/%{KVERREL}/source
/lib/modules/%{KVERREL}/extra
/lib/modules/%{KVERREL}/updates

%files devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-%{_target_cpu}
%endif

%if %{buildsmp}
%files smp
%defattr(-,root,root)
/%{image_install_path}/vmlinuz-%{KVERREL}smp
/boot/System.map-%{KVERREL}smp
/boot/config-%{KVERREL}smp
%dir /lib/modules/%{KVERREL}smp
/lib/modules/%{KVERREL}smp/kernel
/lib/modules/%{KVERREL}smp/build
/lib/modules/%{KVERREL}smp/source
/lib/modules/%{KVERREL}smp/extra
/lib/modules/%{KVERREL}smp/updates

%files smp-devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-smp-%{_target_cpu}
/usr/src/kernels/%{KVERREL}smp-%{_target_cpu}
%endif

%if %{buildxen}
%files xen0
%defattr(-,root,root)
/%{image_install_path}/vmlinuz-%{KVERREL}xen0
/boot/System.map-%{KVERREL}xen0
/boot/config-%{KVERREL}xen0
/boot/xen.gz-%{KVERREL}
/boot/xen-syms-%{KVERREL}
%dir /lib/modules/%{KVERREL}xen0
/lib/modules/%{KVERREL}xen0/kernel
%verify(not mtime) /lib/modules/%{KVERREL}xen0/build
/lib/modules/%{KVERREL}xen0/source
/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf
/lib/modules/%{KVERREL}xen0/extra
/lib/modules/%{KVERREL}xen0/updates

%files xen0-devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-xen0-%{_target_cpu}
/usr/src/kernels/%{KVERREL}xen0-%{_target_cpu}

%files xenU
%defattr(-,root,root)
/%{image_install_path}/vmlinuz-%{KVERREL}xenU
/boot/System.map-%{KVERREL}xenU
/boot/config-%{KVERREL}xenU
%dir /lib/modules/%{KVERREL}xenU
/lib/modules/%{KVERREL}xenU/kernel
%verify(not mtime) /lib/modules/%{KVERREL}xenU/build
/lib/modules/%{KVERREL}xenU/source
/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf
/lib/modules/%{KVERREL}xenU/extra
/lib/modules/%{KVERREL}xenU/updates

%files xenU-devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-xenU-%{_target_cpu}
/usr/src/kernels/%{KVERREL}xenU-%{_target_cpu}
%endif

%if %{buildxenPAE}
%files xen0-PAE
%defattr(-,root,root)
/%{image_install_path}/vmlinuz-%{KVERREL}xen0-PAE
/boot/System.map-%{KVERREL}xen0-PAE
/boot/config-%{KVERREL}xen0-PAE
/boot/xen.gz-%{KVERREL}-PAE
/boot/xen-syms-%{KVERREL}-PAE
%dir /lib/modules/%{KVERREL}xen0-PAE
/lib/modules/%{KVERREL}xen0-PAE/kernel
%verify(not mtime) /lib/modules/%{KVERREL}xen0-PAE/build
/lib/modules/%{KVERREL}xen0-PAE/source
/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf
/lib/modules/%{KVERREL}xen0-PAE/extra
/lib/modules/%{KVERREL}xen0-PAE/updates

%files xen0-PAE-devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-xen0-PAE-%{_target_cpu}
/usr/src/kernels/%{KVERREL}xen0-PAE-%{_target_cpu}

%files xenU-PAE
%defattr(-,root,root)
/%{image_install_path}/vmlinuz-%{KVERREL}xenU-PAE
/boot/System.map-%{KVERREL}xenU-PAE
/boot/config-%{KVERREL}xenU-PAE
%dir /lib/modules/%{KVERREL}xenU-PAE
/lib/modules/%{KVERREL}xenU-PAE/kernel
%verify(not mtime) /lib/modules/%{KVERREL}xenU-PAE/build
/lib/modules/%{KVERREL}xenU-PAE/source
/etc/ld.so.conf.d/kernelcap-%{KVERREL}.conf
/lib/modules/%{KVERREL}xenU-PAE/extra
/lib/modules/%{KVERREL}xenU-PAE/updates

%files xenU-PAE-devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-xenU-PAE-%{_target_cpu}
/usr/src/kernels/%{KVERREL}xenU-PAE-%{_target_cpu}
%endif

%if %{buildkdump}

%files kdump
%defattr(-,root,root)
/%{image_install_path}/vmlinux-%{KVERREL}kdump
/boot/System.map-%{KVERREL}kdump
/boot/config-%{KVERREL}kdump
%dir /lib/modules/%{KVERREL}kdump
/lib/modules/%{KVERREL}kdump/kernel
/lib/modules/%{KVERREL}kdump/build
/lib/modules/%{KVERREL}kdump/source
/lib/modules/%{KVERREL}kdump/extra
/lib/modules/%{KVERREL}kdump/updates

%files kdump-devel
%defattr(-,root,root)
%verify(not mtime) /usr/src/kernels/%{KVERREL}-kdump-%{_target_cpu}
/usr/src/kernels/%{KVERREL}kdump-%{_target_cpu}
%endif

# only some architecture builds need kernel-doc

%if %{builddoc}
%files doc
%defattr(-,root,root)
%{_datadir}/doc/kernel-doc-%{kversion}/Documentation/*
%dir %{_datadir}/doc/kernel-doc-%{kversion}/Documentation
%dir %{_datadir}/doc/kernel-doc-%{kversion}
%endif

%changelog
* Tue Mar 26 2006 Dave Jones <davej@redhat.com>
- 2.6.16.1

* Mon Mar 25 2006 Dave Jones <davej@redhat.com>
- Include patches posted for review for inclusion in 2.6.16.1
- Updated new audit msg types.
- Make acpi-cpufreq 'sticky'
- Fix broken x86-64 32bit vDSO. (#186924)

* Fri Mar 24 2006 Dave Jones <davej@redhat.com>
- Reenable HDLC driver (#186257)
- Reenable ISA NE2000 clones. (#136569)

* Fri Mar 24 2006 David Woodhouse <dwmw2@redhat.com>
- Fix lockup when someone takes the bcm43xx device down while it's
  scanning (#180953)

* Wed Mar 22 2006 David Woodhouse <dwmw2@redhat.com>
- Update the bcm43xx driver to make it work nicely with initscripts
  and NetworkManager without user intervention.

* Tue Mar 21 2006 Dave Jones <davej@redhat.com>
- Improve spinlock scalability on big machines.
- Update exec-shield to Ingo's latest.
  (Incorporates John Reiser's "map the vDSO intelligently" patch
   which increases the efficiency of prelinking - #162797).

* Mon Mar 20 2006 Dave Jones <davej@redhat.com>
- ACPI ecdt uid hack. (#185947)

* Mon Mar 20 2006 Juan Quintela <quintela@redhat.com>
- fix xen vmx in 64 bits.

* Mon Mar 20 2006 Dave Jones <davej@redhat.com>
- 2.6.16

* Sun Mar 19 2006 Dave Jones <davej@redhat.com>
- 2.6.16rc6-git12

* Sat Mar 18 2006 Dave Jones <davej@redhat.com>
- 2.6.16rc6-git10 & git11

* Fri Mar 17 2006 Dave Jones <davej@redhat.com>
- 2.6.16rc6-git8 & git9

* Thu Mar 16 2006 Dave Jones <davej@redhat.com>
- 2.6.16rc6-git7

* Wed Mar 15 2006 Dave Jones <davej@redhat.com>
- 2.6.16rc6-git5
- Unmark 'print_tainted' as a GPL symbol.

* Tue Mar 14 2006 Dave Jones <davej@redhat.com>
- FC5 final kernel
- 2.6.16-rc6-git3

