Name:				ru.sashikknox.hmm3

%ifarch armv7hl
%global build_dir build_armv7hl
%else 
    %ifarch aarch64
        %global build_dir build_aarch64
    %else
        %global build_dir build_x86
    %endif
%endif

%define __provides_exclude_from ^(%{_datadir}/%{name}/lib/.*\\.so.*|%{_datadir}/%{name}/lib/AI/.*\\.so.*)$
%define __requires_exclude ^libboost_.*\\.so.*|libicu.*|libminizip\\.so.*|libvcmi\\.so.*|libwayland-client\\.so.*|libglib-2\\.0.*|ld-linux.*$

Summary:			VCMI is an open-source project aiming to reimplement HoMM3 game engine, giving it new and extended possibilities.
Version:			1.2.1
Release:			3
License:			GPLv2+
Group:				Amusements/Games

%define _unpackaged_files_terminate_build 0



# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  wget https://github.com/vcmi/vcmi/archive/0.99.tar.gz
#  tar -xzf 0.99.tar.gz vcmi-0.99-1
Source:				vcmi-0.99-1.tar.gz
URL:				http://forum.vcmi.eu/portal.php
BuildRequires:		cmake
BuildRequires:		pkgconfig(mce)
BuildRequires:		pkgconfig(wayland-egl)
BuildRequires:		pkgconfig(wayland-client)
BuildRequires:		pkgconfig(wayland-cursor)
BuildRequires:		pkgconfig(wayland-protocols)
BuildRequires:		pkgconfig(wayland-scanner)
BuildRequires:		pkgconfig(egl)
BuildRequires:		pkgconfig(glesv2)
BuildRequires:		pkgconfig(xkbcommon)
BuildRequires:		pkgconfig(libpulse)
BuildRequires:		pkgconfig(audioresource)
BuildRequires:		pkgconfig(glib-2.0)
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
%attr(755,root,root) %{_datadir}/%{name}/lib
%attr(755,root,root) %{_datadir}/%{name}/lib/AI

%changelog

* Fri Jun 08 2012 VCMI - 0.89-1
- Initial version

