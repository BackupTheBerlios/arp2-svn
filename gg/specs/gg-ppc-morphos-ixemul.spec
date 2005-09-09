%define Name ppc-morphos-ixemul-devel
%define Version 49.0

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 1

Summary     	: Includes and libraries for ppc-morphos ixemul development.
Group       	: Development/Libraries
Copyright   	: GPL

Source0		: ppc-morphos-ixemul.tar.gz
BuildRoot   	: /tmp/%{Name}-%{Version}

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

This package is for PowerPC MorphOS development.

%prep
%setup -q -n ppc-morphos-ixemul

%build
%define __libtoolize echo
%define _libdir %{_prefix}/ppc-morphos/lib
%define _includedir %{_prefix}/ppc-morphos/include
%configure --build=%{_build}			\
           --host=ppc-morphos			\
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


%changelog
* Wed Oct  13 2001 Martin Blom <martin@blom.org>
- Initial release.

