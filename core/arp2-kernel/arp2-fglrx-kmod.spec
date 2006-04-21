# stuff to be implemented externally:
Source10: kmodtool
%define   kmodtool sh %{SOURCE10}
# end stuff to be ...

# hardcode for now:
%{!?kversion: %define kversion 2.6.16-1.2096.1}

%define kmod_name fglrx
%define kverrel %(%{kmodtool} verrel %{?kversion} 2>/dev/null)

%define upvar "arp2"
%ifarch i686 ppc
%define smpvar arp2-smp
%endif
%ifarch i686 x86_64
#disabled for now:
#define xenvar xen0
#define kdumpvar kdump
%endif
%{!?kvariants: %define kvariants %{?upvar} %{?smpvar} %{?xenvar} %{?kdumpvar}}


Name:           %{kmod_name}-kmod
Version:        8.23.7
Release:        5.%(echo %{kverrel} | tr - _)
Summary:        ATI proprietary driver for ATI Radeon graphic cards, kernel part

Group:		System Environment/Kernel
License:        Distributable
URL:            http://www.ati.com/support/drivers/linux/radeon-linux.html

Source0:        http://www.leemhuis.info/files/fedorarpms/KMODFILES.lvn/fglrx-kmod-data-%{version}.tar.bz2

Patch1:		ati-fglrx-makefile.diff
Patch2:		ati-fglrx-makesh.diff
Patch10:	ati-fglrx-via_int_agpgart.patch
Patch20:	ati-fglrx-pm_legacy.patch
Patch22:	ati-fglrx-accessok.patch
Patch23:	fglrx-vma_info_fix.patch
Patch24:	fglrx-modparmdesc.patch
Patch25:	fglrx-x86_64-fixups.patch

ExclusiveArch:  i586 i686 x86_64
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# magic hidden here:
%{expand:%(%{kmodtool} rpmtemplate %{kmod_name} %{kverrel} %{kvariants} 2>/dev/null)}

%description
This Package contains the kernel modules required for 3D acceleration with
the ATI proprietary driver for ATI Radeon graphic cards.  This package is
built for kernel %{kernel} (%{_target_cpu}).


%prep
# to understand the magic better or to debug it, uncomment this:
%{kmodtool} rpmtemplate %{kmod_name} %{kverrel} %{kvariants} 2>/dev/null
#sleep 5
%setup -q -c -T -a 0
%ifarch %{ix86}
ln -s fglrxpkg-x86 fglrx
%else
ln -s fglrxpkg-x64 fglrx
%endif
mkdir fglrxpkg
cp -r fglrx/common/* fglrx/arch/*/* fglrxpkg/

for kvariant in %{kvariants} ; do
    cp -a fglrxpkg/ _kmod_build_$kvariant
    pushd _kmod_build_$kvariant
    find lib/modules/fglrx/build_mod/ usr/share/doc  -type d -print0 | xargs -0 chmod 0755
    find lib/modules/fglrx/build_mod/ -type f -print0 | xargs -0 chmod 0644
    pushd lib/modules/fglrx/build_mod/
%patch1 -b .patch1
%patch2 -b .patch2
%patch10 -p1 -b .patch10
%patch22 -p1 -b .patch22
%ifarch x86_64
%patch23 -p1 -b .patch23
%patch24 -p1 -b .patch24
%patch25 -p1 -b .patch25
%endif
    popd
    popd
done


%build
for kvariant in %{kvariants}
do
    ksrc=%{_usrsrc}/kernels/%{kverrel}${kvariant:+-$kvariant}-%{_target_cpu}
    pushd _kmod_build_$kvariant/lib/modules/fglrx/build_mod/
    export AS_USER=y
    export KERNEL_PATH="${ksrc}"
    export FEDORA_UNAME_R="%{kverrel}${kvariant:+-$kvariant}"
    export FEDORA_UNAME_M="%{_target_cpu}"
    export CC="gcc"
    bash make.sh verbose
    popd
done


%install
rm -rf $RPM_BUILD_ROOT
for kvariant in %{kvariants}
do
    install -D -m 0644 _kmod_build_$kvariant/lib/modules/fglrx/build_mod/2.6.x/fglrx.ko $RPM_BUILD_ROOT/lib/modules/%{kverrel}${kvariant}/extra/%{kmod_name}/fglrx.ko
done
chmod u+x $RPM_BUILD_ROOT/lib/modules/*/extra/%{kmod_name}/*


%clean
rm -rf $RPM_BUILD_ROOT


%changelog
* Fri Apr 14 2006 Martin Blom <martin@blom.org> 8.23.7-4.2.6.16_1
- Created from Livna's 8.23.7-4.2.6.16_1.2080_FC5 SRPM.
