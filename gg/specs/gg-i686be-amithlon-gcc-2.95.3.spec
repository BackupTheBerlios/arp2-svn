%define Name gcc
%define Version 2.95.3
%define Target i686be-amithlon

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

This package is for ix86 Amithlon development.


%prep
if [ -r /opt/gg/%{Target}/sys-include ]; then
 echo /opt/gg/%{Target}/sys-include must not exist when building the RPM.
 exit 1
fi
%setup -q -n %{Name}-%{Version}
%patch0 -p1


%build
# Just plain optimizing, since these flags will be passed to the build cross
# compiler as well and the native one. Now if I only could figure out how to
# set XFCFLAGS to "%{optflags}" ....
CFLAGS="-O2"; export CFLAGS
CXXFLAGS="-O2"; export CXXFLAGS
FFLAGS="-O2"; export FFLAGS
%define _target_platform %{Target}
%configure --enable-languages=				\
           --enable-version-specific-runtime-libs	\
           --build=%{_build}				\
           --host=%{_host}
make all


%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall

%clean
rm -rf ${RPM_BUILD_ROOT}


%files
%defattr (-,root,root)
%doc COPYING COPYING.LIB ChangeLog FAQ MAINTAINERS Product-Info README
#%{_bindir}/%{Target}*
#%{_bindir}/%{Target}-c++
#%{_bindir}/%{Target}-g++
%{_bindir}/%{Target}-gcc
%{_bindir}/%{Target}-protoize
%{_bindir}/%{Target}-unprotoize
%{_libdir}/gcc-lib/%{Target}/%{Version}
%{_mandir}/man1/%{Target}*
%{_prefix}/%{Target}/*


#* Sun Jun 30 2002 Martin Blom <martin@blom.org>
#- Release 2.95.3-5.
#- Applied GG patches for libio.
#- Added the C++ compiler (plus includes and libraries) to package.

%changelog
* Thu Jan 20 2005 Martin Blom <martin@blom.org>
- Added -fbaserel(%|32) and -mresident(%|32) support

* Sun Jan  9 2005 Martin Blom <martin@blom.org>
- Bug fixes.

* Sun Jun 30 2002 Martin Blom <martin@blom.org>
- Release 2.95.3-4.
- Fixed incorrect inclusion of sys-include headers.

* Sun Jun 23 2002 Martin Blom <martin@blom.org>
- All known "internal errors" fixed (except for when using ppro
optimization) (2002-04-12 release).

* Fri Feb  8 2002 Martin Blom <martin@blom.org>
- Now supports floating point path and 64 bit integers.

* Mon Dec 10 2001 Martin Blom <martin@blom.org>
- Initial release.
