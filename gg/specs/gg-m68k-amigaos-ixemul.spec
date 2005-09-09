%define Name m68k-amigaos-ixemul-devel
%define Version 48.2

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 1

Summary     	: Includes and libraries for m68k-amigaos ixemul development.
Group       	: Development/Libraries
Copyright   	: GPL

Source0		: m68k-amigaos-ixemul.tar.gz
BuildRoot   	: /tmp/%{Name}-%{Version}

%package -n gg-ixemul-man
Summary     	: Manual pages for ixemul development.
Group       	: Documentation


%description
Essentially it is a BSD-4.3 Unix kernel running under the Amiga OS and
p.OS.  The code for handling Unix signals is taken almost verbatim from the
BSD kernel sources, for example.  Multitasking and file I/O is, of course,
passed on to the underlying OS.  Because the library resembles BSD Unix so
closely, it has made it possible to port almost all Unix programs.

All networking calls are routed through a new library, ixnet.library, which
will pass them on to the appropriate networking package.  Currently
supported are AmiTCP and AS225, Inet-225.

Because of the conformance to BSD, the library is not too conservative with
resources or overly concerned with Amiga standards.  For example, command
line expansion uses Unix semantics and doesn't use ReadArgs().  The purpose
of ixemul.library is to emulate Unix as well as is technically possible.
So given a choice between Amiga behavior or Unix behavior, the last one is
chosen.

This package is for m68k AmigaOS development.


%description -n gg-ixemul-man
Essentially it is a BSD-4.3 Unix kernel running under the Amiga OS and
p.OS.  The code for handling Unix signals is taken almost verbatim from the
BSD kernel sources, for example.  Multitasking and file I/O is, of course,
passed on to the underlying OS.  Because the library resembles BSD Unix so
closely, it has made it possible to port almost all Unix programs.

All networking calls are routed through a new library, ixnet.library, which
will pass them on to the appropriate networking package.  Currently
supported are AmiTCP and AS225, Inet-225.

Because of the conformance to BSD, the library is not too conservative with
resources or overly concerned with Amiga standards.  For example, command
line expansion uses Unix semantics and doesn't use ReadArgs().  The purpose
of ixemul.library is to emulate Unix as well as is technically possible.
So given a choice between Amiga behavior or Unix behavior, the last one is
chosen.

This package contains all ixemul manual pages.


%prep
%setup -q -n m68k-amigaos-ixemul

%build
%define __libtoolize echo
%define _libdir %{_prefix}/m68k-amigaos/lib
%define _includedir %{_prefix}/m68k-amigaos/include
%configure --build=%{_build}			\
           --host=m68k-amigaos
make all

%install
rm -rf ${RPM_BUILD_ROOT}

%makeinstall

%clean
rm -rf ${RPM_BUILD_ROOT}


%files
%defattr(-,root,root)
%{_libdir}/*
%{_includedir}

%files -n gg-ixemul-man
%defattr(-,root,root)
%{_mandir}/cat?/*


%changelog
* Wed Oct  14 2001 Martin Blom <martin@blom.org>
- Initial release.

