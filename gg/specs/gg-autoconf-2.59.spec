%define Name autoconf
%define Version 2.59

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 1

Summary     	: A GNU tool for automatically configuring source code.
Group       	: Development/Tools
Copyright   	: GPL
URL         	: http://sources.redhat.com/autoconf/

Source0		: ftp://ftp.gnu.org/gnu/%{Name}/%{Name}-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 4.0

%description
GNU's Autoconf is a tool for configuring source code and Makefiles.
Using Autoconf, programmers can create portable and configurable
packages, since the person building the package is allowed to
specify various configuration options.

You should install Autoconf if you are developing software and you'd
like to use it to create shell scripts which will configure your
source code packages. If you are installing Autoconf, you will also
need to install the GNU m4 package.

Note that the Autoconf package is not required for the end user who
may be configuring software with an Autoconf-generated script;
Autoconf is only required for the generation of the scripts, not
their use.

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
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/autoconf.info


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/autoconf.info
fi


%files
%defattr(-,root,root)
%doc AUTHORS BUGS ChangeLog* COPYING NEWS Product-Info README THANKS TODO
%{_bindir}/*
%{_infodir}/auto*.info
%{_mandir}/man?/*
%{_datadir}/autoconf
%{_datadir}/emacs/site-lisp/auto*

%changelog
* Sat Jan 15 2005 Martin Blom <martin@blom.org>
- Updated to 2.59.

* Sat Oct 20 2001 Martin Blom <martin@blom.org>
- Rebuild using latest glibc update from RedHat.

* Wed Oct  8 2001 Martin Blom <martin@blom.org>
- Initial release.

