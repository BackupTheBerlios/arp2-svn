%define Name libdebug
%define Version 1.0
%define Target ppc-morphos
%define __os_install_post /usr/lib/rpm/brp-compress; /usr/lib/rpm/brp-strip-comment-note

Name        	: gg-%{Target}-%{Name}-devel
Version     	: %{Version}
Release     	: 2

Summary     	: A debug.lib-style library for %{Target} development.
Group       	: Development/Libraries
Copyright   	: Runtime GPL

Source0		: %{Name}-%{Version}.tar.gz
#Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-sfdc >= 1.3, 
#Requires	: gg-%{Name}-common = %{version}-%{release}

%description
This is libdebug, a static library that implements the standard Amiga
debug function calls such as KPrintF() etc. See <clib/debug_protos.h>
for a complete list.

%prep
%setup -q -n %{Name}-%{Version}
#%patch0 -p1

%build
%define _libdir %{_prefix}/%{Target}/lib
%define _includedir %{_prefix}/%{Target}/include
`pwd`/configure \
        --prefix=%{_prefix} \
        --exec-prefix=%{_exec_prefix} \
        --bindir=%{_bindir} \
        --sbindir=%{_sbindir} \
	--sysconfdir=%{_sysconfdir} \
	--datadir=%{_datadir} \
	--includedir=%{_includedir} \
	--libdir=%{_libdir} \
	--libexecdir=%{_libexecdir} \
	--localstatedir=%{_localstatedir} \
        --sharedstatedir=%{_sharedstatedir} \
        --mandir=%{_mandir} \
        --infodir=%{_infodir} \
	--host=%{Target} --build=%{_build}
make all

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall

%clean
rm -rf ${RPM_BUILD_ROOT}


#%post -n gg-%{Name}-common
#/sbin/install-info --info-dir=%{_infodir} %{_infodir}/libnix.info


#%preun -n gg-%{Name}-common
#if [ $1 = 0 ] ;then
#  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/libnix.info
#fi


%files
%defattr(-,root,root)
%doc COPYING*
%{_libdir}/*

%changelog
* Sun Sep 11 2005 Martin Blom <martin@blom.org>
- Release 1.0-1.
- Rebuilt on CentOS 4.1.

* Sun Jan 23 2005 Martin Blom <martin@blom.org>
- Initial RPM
