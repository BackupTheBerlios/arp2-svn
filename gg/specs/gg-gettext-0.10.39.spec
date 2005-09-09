%define Name gettext
%define Version 0.10.39

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 2

Summary     	: GNU libraries and utilities for producing multi-lingual messages.
Group       	: Development/Tools
Copyright   	: GPL
URL         	: http://www.gnu.org/software/gettext/gettext.html

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 4.0

%description
The GNU gettext package provides a set of tools and documentation for
producing multi-lingual messages in programs.  Tools include a set of
conventions about how programs should be written to support message
catalogs, a directory and file naming organization for the message
catalogs, a runtime library which supports the retrieval of translated
messages, and stand-alone programs for handling the translatable and
the already translated strings.  Gettext provides an easy to use
library and tools for creating, using, and modifying natural language
catalogs and is a powerful and simple method for internationalizing
programs.


%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1

%build
%define __libtoolize echo
%configure
make all MSGFMT=../src/msgfmt GMSGFMT=../src/msgfmt

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall


%clean
rm -rf ${RPM_BUILD_ROOT}


%post
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/gettext.info


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/gettext.info
fi


%files
%defattr(-,root,root)
%doc ABOUT-NLS AUTHORS BUGS ChangeLog* COPYING DISCLAIM NEWS Product-Info README* THANKS TODO
%{_bindir}/*
%{_infodir}/gettext*info*
%{_mandir}/man?/*
%{_datadir}/aclocal/*
%{_datadir}/emacs/site-lisp/*
%{_datadir}/gettext
%{_datadir}/locale/*/*/*

%changelog
* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  11 2001 Martin Blom <martin@blom.org>
- Initial release.

