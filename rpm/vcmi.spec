
%if "0%{?auroraos}" != "0"
Name:				ru.sashikknox.hmm3
%else
Name:				vcmi
%endif

# %define __provides_exclude_from ^(%{_datadir}/%{name}/lib/.*\\.so.*|%{_datadir}/%{name}/lib/AI/.*\\.so.*)$
%define __requires_exclude ^libboost_.*\\.so.*|libicu.*|libminizip\\.so|libvcmi\\.so$

Summary:			VCMI is an open-source project aiming to reimplement HoMM3 game engine, giving it new and extended possibilities.
Version:			0.99
Release:			1%{?dist}
License:			GPLv2+
Group:				Amusements/Games

%define _unpackaged_files_terminate_build 0

%ifarch armv7hl
%global build_dir build_armv7hl
%else 
    %ifarch aarch64
        %global build_dir build_aarch64
    %else
        %global build_dir build_x86
    %endif
%endif

# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  wget https://github.com/vcmi/vcmi/archive/0.99.tar.gz
#  tar -xzf 0.99.tar.gz vcmi-0.99-1
Source:				vcmi-0.99-1.tar.gz
URL:				http://forum.vcmi.eu/portal.php
BuildRequires:		cmake
BuildRequires:		SDL2-devel
BuildRequires:		SDL2_image-devel
BuildRequires:		SDL2_ttf-devel
BuildRequires:		SDL2_mixer-devel
BuildRequires:		boost-devel >= 1.51
BuildRequires:		boost-filesystem >= 1.51
BuildRequires:		boost-iostreams >= 1.51
BuildRequires:		boost-system >= 1.51
BuildRequires:		boost-thread >= 1.51
BuildRequires:		boost-program-options >= 1.51
BuildRequires:		boost-locale >= 1.51
BuildRequires:		zlib-devel
BuildRequires:		rsync

%description
VCMI is an open-source project aiming to reimplement HoMM3 game engine, giving it new and extended possibilities.

%prep
%setup -q -n %{name}-%{version}-1
# >> setup
# << setup

%build
mkdir -p %{build_dir}
cd %{build_dir}
cmake -DCMAKE_BUILD_TYPE=Relelese -DAURORAOS=ON -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_TEST=0 ..
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
cd %{build_dir}
/usr/bin/bash ../rpm/build_aurora.sh %{buildroot} %{name}


%files
%{_bindir}/%{name}
%{_datadir}/%{name}/*
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog

* Tue Nov 01 2016 VCMI - 0.99-1
- New upstream release

* Wed Apr 01 2015 VCMI - 0.98-1
- New upstream release

* Sun Nov 02 2014 VCMI - 0.97-1
- New upstream release

* Tue Jul 01 2014 VCMI - 0.96-1
- New upstream release

* Sat Mar 01 2014 VCMI - 0.95-1
- New upstream release

* Wed Oct 02 2013 VCMI - 0.94-1
- New upstream release

* Sun Jun 02 2013 VCMI - 0.93-1
- New upstream release

* Wed Mar 06 2013 VCMI - 0.92-1
- New upstream release

* Fri Feb 01 2013 VCMI - 0.91-2
- New upstream release

* Wed Jan 30 2013 VCMI - 0.91-1
- Development release

* Sun Oct 21 2012 VCMI - 0.90-2
- Second release of 0.90, Fixed battles crash

* Sat Oct 06 2012 VCMI - 0.90-1
- New upstream release

* Fri Jun 08 2012 VCMI - 0.89-1
- Initial version

