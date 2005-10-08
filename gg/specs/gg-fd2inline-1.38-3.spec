%define Name fd2inline
%define Version 1.38
%define Release 3

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: %{Release}

Summary     	: Convert FD files to gcc 'inlines'
Group       	: Development/Tools
Copyright   	: GPL
#URL         	: http://sources.redhat.com/autoconf/

Source0		: ftp://ftp.geekgadgets.org/pub/geekgadgets/baseline/%{Name}-1.21-src.tgz
Patch0		: gg-%{Name}-1.21-%{Version}-%{Release}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 4.0

%description
FD2InLine is useful if you want to use GCC for Amiga-specific
development and would like to call the functions in the Amiga shared
libraries efficiently.  The format of calls to the Amiga shared
library functions differs substantially from the default function call
format of C compilers.  Therefore, some tricks are necessary if you
want to use these functions.

FD2InLine is a parser that converts FD files and clib files to GCC
inlines.  FD and clib files contain information about functions in
shared libraries.  FD2InLine reads these two files and merges the
information contained therein, producing an output file suitable for
use with the GCC compiler.  This output file contains so-called
inlines, one for each function entry.  Using them, GCC can produce
very efficient code for making function calls to Amiga shared
libraries.

%prep
%setup -q -n %{Name}-1.21
%patch0 -p1

%build
%configure
make all -f Makefile.cross

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall -f Makefile.cross


%clean
rm -rf ${RPM_BUILD_ROOT}


%post
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/fd2inline.info


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/fd2inline.info
fi


%files
%defattr(-,root,root)
%doc COPYING Product-Info
%{_bindir}/*
%{_infodir}/fd2inline*info*
%{_datadir}/fd2inline

%changelog
* Tue Sep 20 2005 Martin Blom <martin@blom.org> - 
- Updated macros.h for m68k to include LP5A4().

* Sun Jul 27 2003 Martin Blom <martin@blom.org>
- Nuked include file hierarchy (now in gg-fd2sfd)
- Renamed gg-fix-includes to gg-fix-includes.fd2inline.

* Sat May 24 2003 Martin Blom <martin@blom.org>
- Version 1.38

* Sun Apr 13 2003 Martin Blom <martin@blom.org>
- Version 1.37

* Fri Feb  8 2002 Martin Blom <martin@blom.org>
- Version 1.35

* Mon Dec 10 2001 Martin Blom <martin@blom.org>
- Version 1.32.

* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.
- Version 1.31.

* Wed Oct  8 2001 Martin Blom <martin@blom.org>
- Initial release.

