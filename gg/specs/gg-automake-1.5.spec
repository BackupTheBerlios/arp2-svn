%define Name automake
%define Version 1.5

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 2

Summary     	: A GNU tool for automatically creating Makefiles.
Group       	: Development/Tools
Copyright   	: GPL
URL         	: http://sources.redhat.com/automake/

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 4.0

%description
Automake is an experimental Makefile generator. Automake was inspired
by the 4.4BSD make and include files, but aims to be portable and to
conform to the GNU standards for Makefile variables and targets.

You should install Automake if you are developing software and would
like to use its capabilities of automatically generating GNU
standard Makefiles. if you install Automake, you will also need to
install GNU's Autoconf package.

%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1

%build
%configure
make all

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall


%clean
rm -rf ${RPM_BUILD_ROOT}


%post
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/automake.info


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/automake.info
fi


%files
%defattr(-,root,root)
%doc ChangeLog* COPYING NEWS Product-Info README* THANKS TODO
%{_bindir}/*
%{_infodir}/automake*info*
%{_datadir}/*

%changelog
* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  8 2001 Martin Blom <martin@blom.org>
- Initial release.

