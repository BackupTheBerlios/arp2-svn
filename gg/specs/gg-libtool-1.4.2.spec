%define Name libtool
%define Version 1.4.2

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 2

Summary     	: The GNU libtool, which simplifies the use of shared libraries.
Group       	: Development/Tools
Copyright   	: GPL
URL         	: http://www.gnu.org/software/libtool/libtool.html

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 4.0

%description
The libtool package contains the GNU libtool, a set of shell scripts
which automatically configure UNIX and UNIX-like architectures to
generically build shared libraries.  Libtool provides a consistent,
portable interface which simplifies the process of using shared
libraries.

If you are developing programs which will use shared libraries, you
should install libtool.


%prep
%setup -q -n %{Name}-%{Version}
%patch0 -p1

%build
./configure \
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
        --infodir=%{_infodir}
make all

%install
rm -rf ${RPM_BUILD_ROOT}
%makeinstall


%clean
rm -rf ${RPM_BUILD_ROOT}


%post
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/libtool.info


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/libtool.info
fi


%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog* COPYING NEWS Product-Info README* THANKS TODO*
%{_bindir}/*
%{_includedir}/*
%{_libdir}/*
%{_infodir}/libtool*info*
%{_datadir}/aclocal/*
%{_datadir}/libtool

%changelog
* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  13 2001 Martin Blom <martin@blom.org>
- Initial release.

