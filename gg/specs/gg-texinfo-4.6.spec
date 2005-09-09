%define Name texinfo
%define Version 4.6

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 1

Summary     	: Tools needed to create Texinfo format documentation files.
Group       	: Applications/Publishing
Copyright   	: GPL
URL         	: http://www.gnu.org/software/texinfo/texinfo.html

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires	: gg-autoconf, gg-automake, gg-gettext

%description
Texinfo is a documentation system that can produce both online
information and printed output from a single source file.  The GNU
Project uses the Texinfo file format for most of its documentation.

Install texinfo if you want a documentation system for producing both
online and print documentation from the same source file and/or if you
are going to write documentation for the GNU Project.

%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1

%build
%configure --enable-amigaguide
make all

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall


%clean
rm -rf ${RPM_BUILD_ROOT}


%post
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/info-stnd.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/info.info
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/texinfo


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/info-stnd.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/info.info
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/texinfo
fi


%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INTRODUCTION NEWS Product-Info README TODO
%{_bindir}/*
%{_infodir}/*info*
%{_datadir}/locale/*/*/*

%changelog
* Sat Jan 15 2005 Martin Blom <martin@blom.org>
- Upgraded to Gunther Nikl's 4.6 patches.

* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  3 2001 Martin Blom <martin@blom.org>
- Initial release.

