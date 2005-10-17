%define Name binutils
%define Version 2.9.1
%define Target m68k-amigaos

Name        	: gg-%{Target}-%{Name}
Version     	: %{Version}
Release     	: 5

Summary     	: A GNU collection of binary utilities for %{Target}.
Group       	: Development/Tools
Copyright   	: GPL
URL         	: http://sources.redhat.com/binutils/

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires	: gg-texinfo
Requires	: gg-binutils-common = %{version}-%{release}

%package -n gg-binutils-common
Summary     	: A GNU collection of binary utilities, common for all targets.
Group       	: Development/Tools


%description
Binutils is a collection of binary utilities, including ar (for creating,
modifying and extracting from archives), nm (for listing symbols from
object files), objcopy (for copying and translating object files),
objdump (for displaying information from object files), ranlib (for
generating an index for the contents of an archive), size (for listing
the section sizes of an object or archive file), strings (for listing
printable strings from files), strip (for discarding symbols), c++filt
(a filter for demangling encoded C++ symbols), addr2line (for converting
addresses to file and line), and nlmconv (for converting object code into
an NLM). 

Install binutils if you need to perform any of these types of actions on
binary files.  Most programmers will want to install binutils.

This package is for m68k AmigaOS development.


%description -n gg-binutils-common
Binutils is a collection of binary utilities, including ar (for creating,
modifying and extracting from archives), nm (for listing symbols from
object files), objcopy (for copying and translating object files),
objdump (for displaying information from object files), ranlib (for
generating an index for the contents of an archive), size (for listing
the section sizes of an object or archive file), strings (for listing
printable strings from files), strip (for discarding symbols), c++filt
(a filter for demangling encoded C++ symbols), addr2line (for converting
addresses to file and line), and nlmconv (for converting object code into
an NLM). 

Install binutils if you need to perform any of these types of actions on
binary files.  Most programmers will want to install binutils.

This package contains files common for all supported targets.


%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1

%build
# Binutils come with its own custom libtool
%define __libtoolize echo
%define _target_platform %{Target}
%define _program_prefix %{Target}-
%configure --enable-shared			\
           --enable-targets="i686be-amithlon ppc-morphos"
make all info

%install
rm -rf ${RPM_BUILD_ROOT}

%makeinstall install-info
install -m 644 include/libiberty.h ${RPM_BUILD_ROOT}%{_prefix}/include

# We don't want to package these files
rm -f ${RPM_BUILD_ROOT}/opt/gg/guide/standards.guide
rm -f ${RPM_BUILD_ROOT}/opt/gg/info/dir


%clean
rm -rf ${RPM_BUILD_ROOT}


%post -n gg-binutils-common
/sbin/ldconfig
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/as.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/bfd.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/binutils.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/gasp.info
#/sbin/install-info --info-dir=%{_infodir} %{_infodir}/gprof.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/ld.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/standards.info


%preun -n gg-binutils-common
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/as.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/bfd.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/binutils.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/gasp.info
#  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/gprof.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/ld.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/standards.info
fi

%postun -n gg-binutils-common -p /sbin/ldconfig


%files
%defattr(-,root,root)
%doc README COPYING COPYING.LIB ChangeLog Product-Info
%{_bindir}/*
%{_mandir}/man1/*
%{_prefix}/%{Target}

%files -n gg-binutils-common
%defattr(-,root,root)
%doc README COPYING COPYING.LIB ChangeLog Product-Info
%{_includedir}/*
%{_infodir}/*info*
%{_libdir}/*

%changelog
* Fri Sep  9 2005 Martin Blom <martin@blom.org>
- Release 2.9.1-5.
- Rebuilt on CentOS 4.1.

* Mon Dec 10 2001 Martin Blom <martin@blom.org>
- Added i686be-amithlon target

* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  3 2001 Martin Blom <martin@blom.org>
- Initial release.

