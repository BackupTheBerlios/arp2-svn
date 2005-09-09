%define Name libnix
%define Version 2.0
%define Target i686be-amithlon
%define __spec_install_post /usr/lib/rpm/brp-compress

Name        	: gg-%{Target}-%{Name}-devel
Version     	: %{Version}
Release     	: 8

Summary     	: A library for %{Target} specific gcc development.
Group       	: Development/Libraries
Copyright   	: Runtime GPL

Source0		: nix_src.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 4.0, gg-%{Target}-%{Name}-devel
Requires	: gg-%{Name}-common = %{version}-%{release}

%description
This is libnix, a static (i.e. link) library for gcc 2.3.3 or above.
It's not a replacement for ixemul.library (though it's possible to
recompile most of the gcc environment with libnix) but a good thing
for amiga specific development on gcc:

* It's mostly compatible to SAS's way of handling things, i.e.
  you get even an automatic shared library opening feature and
  some other things you may miss in ixemul.library.
  This also means it's ANSI compliant.

* It doesn't need any shared libraries than normal Amiga OS ones.

* It is not copyrighted by the FSF. Therefore you neither need
  to include sources nor objects together with your executable.
  (read the GLGPL _before_ flaming on this statement)

* And it's short! I was able to compile a 492 byte 'hello, world'
  using normal main.

* It uses OS20 features whenever necessary.

To cut it short:

Use ixemul.library for porting Un*x programs, libnix for compiling
amiga-only programs and gcc becomes one of the best Amiga compilers.

This package is for ix86 Amithlon development.

%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1
chmod +x mkinstalldirs

%build
%define _libdir %{_prefix}/%{Target}/lib
%define _includedir %{_prefix}/%{Target}/include
LDFLAGS='-r -noixemul' `pwd`/configure \
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
make all libamiga

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall
# Fix libamiga.a:
mv ${RPM_BUILD_ROOT}/%{_prefix}/%{Target}/lib/libnix/libamiga.a ${RPM_BUILD_ROOT}/%{_prefix}/%{Target}/lib/
test -r ${RPM_BUILD_ROOT}/%{_prefix}/%{Target}/lib/libb/libnix/libamiga.a && mv ${RPM_BUILD_ROOT}/%{_prefix}/%{Target}/lib/libb/libnix/libamiga.a ${RPM_BUILD_ROOT}/%{_prefix}/%{Target}/lib/libb/


%clean
rm -rf ${RPM_BUILD_ROOT}


%files
%defattr(-,root,root)
%doc Product-Info README* COPYING* sources/History
%{_libdir}/*
%{_includedir}/*

%changelog
* Mon Jan 10 2005 Martin Blom <martin@blom.org>
- Fixed a problem in asinh() that prevented libm.a from building correctly.
- Enforcer hits when using bad FDs removed from libsocket.a

* Sun Dec 19 2004 Martin Blom <martin@blom.org>
- Fixed several bugs in libsocket.a

* Sat Dec  4 2004 Martin Blom <martin@blom.org>
- memcpy() does not use SysBase anymore.
- Added UserFilter to libamiga.a.
- SetSuperAttrs was broken in the m68k version.

* Tue Jan 21 2003 Martin Blom <martin@blom.org>
- Added patches relating to setjmp/longjmp, symbol aliases, 68881,
  __swbuf, getcwd, lx_select/ix_select, crt0-ix86be.c, ncrt0.S and
  nrcrt0.S.

* Fri Feb  8 2002 Martin Blom <martin@blom.org>
- Now based on the final v2.0.
- Added a couple of math functions.

* Mon Dec 10 2001 Martin Blom <martin@blom.org>
- Initial release.
