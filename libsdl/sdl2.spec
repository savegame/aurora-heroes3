Summary: Simple DirectMedia Layer 2
Name: SDL2
Version:    2.0.12+git1
Release: 1.5.1.jolla
Source: http://www.libsdl.org/release/%{name}-%{version}.tar.gz
URL: http://www.libsdl.org/
License: zlib
BuildRequires: pkgconfig(wayland-egl)
BuildRequires: pkgconfig(wayland-client)
BuildRequires: pkgconfig(wayland-cursor)
BuildRequires: pkgconfig(wayland-protocols)
BuildRequires: pkgconfig(wayland-scanner)
BuildRequires: pkgconfig(egl)
BuildRequires: pkgconfig(glesv1_cm)
BuildRequires: pkgconfig(glesv2)
BuildRequires: pkgconfig(xkbcommon)
BuildRequires: pkgconfig(libpulse-simple)

Patch0: sdl2-add-support-for-orientation-in-wayland.patch
Patch1: sdl2-wayland-input-fix.patch

%description
This is the Simple DirectMedia Layer, a generic API that provides low
level access to audio, keyboard, mouse, and display framebuffer across
multiple platforms.

%package devel
Summary: Simple DirectMedia Layer 2 - Development libraries
Requires: %{name} = %{version}

%description devel
This is the Simple DirectMedia Layer, a generic API that provides low
level access to audio, keyboard, mouse, and display framebuffer across
multiple platforms.

This is the libraries, include files and other resources you can use
to develop SDL applications.


%prep
%autosetup -p1 -n %{name}-%{version}/%{name}

%build
%configure CFLAGS='-std=c99' --disable-video-x11 --enable-video-wayland --enable-pulseaudio
make %{?_smp_mflags}

%install
%make_install

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/lib*.so.*

%files devel
%defattr(-,root,root,-)
%{_bindir}/*-config
%{_libdir}/lib*.so
%{_libdir}/cmake/SDL2/*.cmake
%{_includedir}/*/*.h
%{_libdir}/pkgconfig/*
%{_datadir}/aclocal/*
%changelog
* Mon May 11 2020 Matti Lehtimäki <matti.lehtimaki@jolla.com> - 2.0.12+git1
- [sdl] Update to 2.0.12 release. JB#49887
* Fri May  3 2019 Matti Lehtimäki <matti.lehtimaki@jolla.com> - 2.0.9+git3
- [sdl] Add upstream patch to not force X11 in EGL. Contributes to JB#37662
* Fri Mar 29 2019 Matti Lehtimäki <matti.lehtimaki@jolla.com> - 2.0.9+git2
- [sdl] Update libsdl to 2.0.9. Fixes MER#1920
* Wed Aug 22 2018 Matti Kosola <matti.kosola@jollamobile.com> - 2.0.3-nemo4
- [sdl] Fix "unresponsible application" issue. Contributes MER#1934
* Thu Nov 24 2016 pvuorela <pekka.vuorela@jolla.com> - 2.0.3-nemo3
- [sdl] Set orientation and window flags via SDL hints. Fixes JB#27386
* Mon Apr 21 2014 Thomas Perl <m@thp.io> - 2.0.3-nemo2
- [nemo] Add RPM changes entries from old branch
- [nemo] Rebased RPM packaging on upstream 2.0.3 tag
* Fri Apr 18 2014 Thomas Perl <thomas.perl@jolla.com> - 2.0.3-nemo1
- [nemo] New upstream release 2.0.3
- [nemo] SDL 2.0.3 RPM packaging
- [nemo] Wayland: Resize windows with 0x0 requested size to screen size
* Mon Dec 30 2013 Thomas Perl <thomas.perl@jolla.com> - 2.0.1-nemo2
- [nemo] Add RPM packaging
