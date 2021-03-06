%define Name gcc
%define Version 2.95.3
%define Target ppc-morphos
%define __os_install_post /usr/lib/rpm/brp-compress; /usr/lib/rpm/brp-strip; /usr/lib/rpm/brp-strip-comment-note

Name        	: gg-%{Target}-%{Name}
Version     	: %{Version}
Release     	: 6

Summary     	: Various compilers (C, C++, Objective-C, Chill, ...) for %{Target}.
Group       	: Development/Languages
Copyright   	: GPL
URL         	: http://gcc.gnu.org

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires	: gg-texinfo, gg-libtool, gg-%{Target}-binutils >= 2.9.1, gg-%{Target}-ixemul-devel

Requires	: gg-%{Target}-binutils >= 2.9.1
Requires	: gg-gcc-common = %{version}-%{release}

%description
The gcc package contains the GNU Compiler Collection: cc and gcc. You'll need
this package in order to compile C/C++ code.

This package is for PowerPC MorphOS development.


%prep
if [ -r %{_prefix}/%{Target}/sys-include ]; then
 echo %{_prefix}/%{Target}/sys-include must not exist when building the RPM.
 exit 1
fi
%setup -q -n %{Name}-%{Version}
%patch0 -p1

# Make sure c-parse.c and c-parse.h won't be regenerated with a modern bison
touch gcc/c-parse.c
touch gcc/c-parse.h


%build
# Just plain optimizing, since these flags will be passed to the build cross
# compiler as well and the native one. Now if I only could figure out how to
# set XFCFLAGS to "%{optflags}" ....
CFLAGS="-O2"; export CFLAGS
CXXFLAGS="-O2"; export CXXFLAGS
FFLAGS="-O2"; export FFLAGS
%define _target_platform %{Target}
%define _program_prefix %{Target}-
%configure --enable-languages=c++			\
           --enable-version-specific-runtime-libs
make all


%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall

# We don't want to package these files
rm -f ${RPM_BUILD_ROOT}%{_bindir}/cpp
rm -f ${RPM_BUILD_ROOT}%{_bindir}/gcov
rm -f ${RPM_BUILD_ROOT}%{_bindir}/%{Target}-c++filt
rm -f ${RPM_BUILD_ROOT}%{_libdir}/libiberty.a
rm -rf ${RPM_BUILD_ROOT}/%{_infodir}/
rm -rf ${RPM_BUILD_ROOT}/%{_mandir}/man1/cccp*


%clean
rm -rf ${RPM_BUILD_ROOT}


%files
%defattr (-,root,root)
%doc COPYING COPYING.LIB ChangeLog FAQ MAINTAINERS Product-Info README
#%{_bindir}/%{Target}*
%{_bindir}/%{Target}-c++
%{_bindir}/%{Target}-g++
%{_bindir}/%{Target}-gcc
%{_bindir}/%{Target}-protoize
%{_bindir}/%{Target}-unprotoize
%{_libdir}/gcc-lib/%{Target}/%{Version}
%{_mandir}/man1/%{Target}*
%{_prefix}/%{Target}/*


%changelog
* Sun Sep 11 2005 Martin Blom <martin@blom.org> - 
- Release 2.95.3-6.
- Rebuilt on CentOS 4.1.
- Added 'iptr' attribute for pointers and integers. Define
  __HAVE_IPTR_ATTR__.

* Sun Jun 30 2002 Martin Blom <martin@blom.org>
- Release 2.95.3-5.
- Applied GG patches for libio.
- Added the C++ compiler (plus includes and libraries) to package.

* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  13 2001 Martin Blom <martin@blom.org>
- Initial release.

