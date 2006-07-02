# stuff to be implemented externally:
Source10: kmodtool
%define   kmodtool bash %{SOURCE10}
# end stuff to be ...

# hardcode for now:
%{!?kversion: %define kversion 2.6.17-1.2139.1}

%define kmod_name nvidia
%define kverrel %(%{kmodtool} verrel %{?kversion} 2>/dev/null)

%define upvar "arp2"
%ifarch i586 i686
%define smpvar arp2-smp
%endif
%ifarch i686 x86_64
#disabled for now:
#define xenvar xen0
#define kdumpvar kdump
%endif
%{!?kvariants: %define kvariants %{?upvar} %{?smpvar} %{?xenvar} %{?kdumpvar}}


Name:       %{kmod_name}-kmod
Version:    1.0.8762
Release:    1.%(echo %{kverrel} | tr - _)
Summary:    Nvidia Display Driver kernel-module

License:    Distributable
Group:      System Environment/Kernel
URL:        http://www.nvidia.com/

#Source is created from these files:
# ftp://download.nvidia.com/XFree86/Linux-x86/1.0-8762/NVIDIA-Linux-x86-1.0-8762-pkg0.run
# ftp://download.nvidia.com/XFree86/Linux-x86_64/1.0-8762/NVIDIA-Linux-x86_64-1.0-8762-pkg2.run
Source0:	nvidia-glx-kmod-data-%{version}.tar.bz2	
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
ExclusiveArch:	i586 i686 x86_64

%description 
This package provides Nvidia hardware accelerated OpenGL support
for Nvidia chipsets from Geforce2, Quadro4 or newer.

# magic hidden here:
%{expand:%(%{kmodtool} rpmtemplate %{kmod_name} %{kverrel} %{kvariants} 2>/dev/null)}

%prep
# to understand the magic better or to debug it, uncomment this:
%{kmodtool} rpmtemplate %{kmod_name} %{kverrel} %{kvariants} 2>/dev/null
#sleep 5
%setup -q -c -T -a 0
for kvariant in %{kvariants} ; do
%ifarch %{ix86}
    cp -a nvidiapkg-x86 _kmod_build_$kvariant
%else
    cp -a nvidiapkg-x64 _kmod_build_$kvariant
%endif
done


%build
for kvariant in %{kvariants}
do
    ksrc=%{_usrsrc}/kernels/%{kverrel}${kvariant:+-$kvariant}-%{_target_cpu}
    pushd _kmod_build_$kvariant/usr/src/nv/
    ln -s -f Makefile.kbuild Makefile
    make SYSSRC="${ksrc}" module
    popd
done


%install
rm -rf $RPM_BUILD_ROOT
for kvariant in %{kvariants}
do
    install -D -m 0644 _kmod_build_$kvariant/usr/src/nv/nvidia.ko $RPM_BUILD_ROOT/lib/modules/%{kverrel}${kvariant}/extra/%{kmod_name}/nvidia.ko
done
chmod u+x $RPM_BUILD_ROOT/lib/modules/*/extra/%{kmod_name}/*


%clean
rm -rf $RPM_BUILD_ROOT


%changelog
* Sun Jul 2 2006 Martin Blom <martin@blom.org> - 1.0.8762-1-1.2.6.17.2139
- Updated for Fedora's 2.6.17-1.2139_FC5 kernel.

* Fri Jun 16 2006 Martin Blom <martin@blom.org> - 1.0.8762-1-1.2.6.16.2133
- Updated for Fedora's 2.6.16-1.2133_FC5 kernel.

* Thu May 25 2006 Martin Blom <martin@blom.org> - 1.0.8762-1-1.2.6.16.2122
- Updated to Livna's 1.0.8762-1.2.6.16_1.2122_FC5 SRPM

* Mon Apr 10 2006 Martin Blom <martin@blom.org> - 1.0.8756-1-1.2.6.16.2080
- Created from Livna's 1.0.8756-1.2.6.16_1.2080_FC5 SRPM.
