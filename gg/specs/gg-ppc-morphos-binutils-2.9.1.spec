%define Name binutils
%define Version 2.9.1
%define Target ppc-morphos

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

This package is for PowerPC MorphOS development.


%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1

%build
# Binutils come with its own custom libtool
%define __libtoolize echo
%define _target_platform %{Target}
%define _program_prefix %{Target}-
%configure --enable-shared                      \
           --enable-targets="i686be-amithlon m68k-amigaos"
make all info

%install
rm -rf ${RPM_BUILD_ROOT}

%makeinstall install-info
install -m 644 include/libiberty.h ${RPM_BUILD_ROOT}%{_prefix}/include

# We don't want to package these files
rm -f ${RPM_BUILD_ROOT}/opt/gg/guide/standards.guide
rm -f ${RPM_BUILD_ROOT}/opt/gg/info/dir
rm -rf ${RPM_BUILD_ROOT}/opt/gg/include
rm -rf ${RPM_BUILD_ROOT}/opt/gg/info
rm -rf ${RPM_BUILD_ROOT}/opt/gg/lib


%clean
rm -rf ${RPM_BUILD_ROOT}


%files
%defattr(-,root,root)
%doc README COPYING COPYING.LIB ChangeLog Product-Info
%{_bindir}/*
%{_mandir}/man1/*
%{_prefix}/%{Target}


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

