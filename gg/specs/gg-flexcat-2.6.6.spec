%define Name flexcat
%define Version 2.6.6.20050112

Name        	: gg-%{Name}
Version     	: %{Version}
Release     	: 1

Summary     	: The flexible catalog generator
Group       	: Development/Tools
Copyright   	: GPL
URL         	: http://amiga.com.pl/flexcat/

Source0		: flexcat-%{Version}.tar.gz
Patch0		: gg-%{Name}-%{Version}.patch
BuildRoot   	: /tmp/%{Name}-%{Version}

BuildRequires   : gg-texinfo >= 3.12, lha, fileutils

%description

FlexCat is a program which generates catalogs and the source to handle
them. FlexCat works similar to CatComp and KitCat, but differs in
generating any source you want. This is done by using the so called
Source descriptions, which are a template for the code to
generate. They can be edited and hence adapted to any programming
language and individual needs. (Hopefully!)

%prep
%setup -q -n %{Name}-2005-01-12
#lha x FlexCat_Src.lha
#lha x FlexCat_CatSrc.lha
#ln -s flexcat_cat.h.unix flexcat_cat.h
%patch0 -p1

%build
make
makeinfo docs/FlexCat_english.texinfo -o flexcat.info
#gcc ${RPM_OPT_FLAGS} flexcat.c -o flexcat
#makeinfo Catalogs_Src/FlexCat_english.texinfo  -o flexcat.info

%install
rm -rf ${RPM_BUILD_ROOT}
install -d ${RPM_BUILD_ROOT}/%{_bindir} ${RPM_BUILD_ROOT}/%{_infodir}
install -s flexcat ${RPM_BUILD_ROOT}/%{_bindir}
install -m 644 flexcat.info ${RPM_BUILD_ROOT}/%{_infodir}


%clean
rm -rf ${RPM_BUILD_ROOT}


%post
/sbin/install-info --info-dir=%{_infodir} %{_infodir}/flexcat.info


%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/flexcat.info
fi


%files
%defattr(-,root,root)
%doc developer.readme FlexCat.announce FlexCat.history todo
%{_bindir}/*
%{_infodir}/flexcat*info*

%changelog
* Mon May  5 2003 Martin Blom <martin@blom.org>
- Updated to version 2.6.

* Fri Feb  8 2002 Martin Blom <martin@blom.org>
- Initial release.
