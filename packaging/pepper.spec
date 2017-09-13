Name:		pepper
Version:	1.0.19
Release:	0
Summary:	Library for developing wayland compositor
License:	MIT
Group:		Graphics & UI Framework/Wayland Window System

Source:		%{name}-%{version}.tar.xz
source1001:     %name.manifest

%define ENABLE_TDM	1

BuildRequires:	autoconf > 2.64
BuildRequires:	automake >= 1.11
BuildRequires:	libtool >= 2.2
BuildRequires:	pkgconfig
BuildRequires:	xz
BuildRequires:	pkgconfig(wayland-server)
BuildRequires:	pkgconfig(pixman-1)
BuildRequires:	pkgconfig(libinput)
BuildRequires:	pkgconfig(libdrm)
BuildRequires:	pkgconfig(gbm)
BuildRequires:	pkgconfig(egl)
BuildRequires:	pkgconfig(glesv2)
BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:	doxygen
BuildRequires:	pkgconfig(wayland-tbm-client)
BuildRequires:  pkgconfig(wayland-tbm-server)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(tizen-extension-server)
BuildRequires:  pkgconfig(tizen-extension-client)
%if "%{ENABLE_TDM}" == "1"
BuildRequires:  pkgconfig(libtdm)
%endif

%description
Pepper is a lightweight and flexible library for developing various types of wayland compositors.

###### pepper-devel
%package devel
Summary: Development module for pepper package
Requires: %{name} = %{version}-%{release}

%description devel
This package includes developer files common to all packages.

###### keyrouter
%package keyrouter
Summary: Keyrouter module for pepper package

%description keyrouter
This package includes keyrouter module files.

###### keyrouter-devel
%package keyrouter-devel
Summary: Keyrouter development module for pepper package
Requires: pepper-keyrouter = %{version}-%{release}

%description keyrouter-devel
This package includes keyrouter development module files.

###### evdev
%package evdev
Summary: evdev module for pepper package

%description evdev
This package includes evdev module files.

###### evdev-devel
%package evdev-devel
Summary: Evdev development module for pepper package
Requires: pepper-evdev = %{version}-%{release}

%description evdev-devel
This package includes evdev development module files.

###### xkb
%package xkb
Summary: xkb module for pepper package

%description xkb
This package includes xkb module files.

###### xkb-devel
%package xkb-devel
Summary: XKB development module for pepper package
Requires: pepper-xkb = %{version}-%{release}

%description xkb-devel
This package includes xkb development module files.

###### libinput
%package libinput
Summary: Libinput module for pepper package

%description libinput
This package includes libinput module files.

###### libinput-devel
%package libinput-devel
Summary: Libinput development module for pepper package
Requires: pepper-libinput = %{version}-%{release}

%description libinput-devel
This package includes libinput development module files.

###### desktop-shell
%package desktop-shell
Summary: Desktop-shell module for pepper package

%description desktop-shell
This package includes desktop-shell module files.

###### desktop-shell-devel
%package desktop-shell-devel
Summary: Desktop-shell development module for pepper package
Requires: pepper-desktop-shell = %{version}-%{release}
Requires: pepper-xkb = %{version}-%{release}

%description desktop-shell-devel
This package includes desktop-shell development module files.

###### render
%package render
Summary: Render module for pepper package

%description render
This package includes render module files.

###### render-devel
%package render-devel
Summary: Render development module for pepper package
Requires: pepper-render = %{version}-%{release}

%description render-devel
This package includes render development module files.

###### drm backend
%package drm
Summary: Drm backend module for pepper package

%description drm
This package includes drm backend module files.

###### drm backend devel
%package drm-devel
Summary: Drm backend development module for pepper package
Requires: pepper-drm = %{version}-%{release}

%description drm-devel
This package includes drm backend development module files.

###### tdm backend
%package tdm
Summary: TDM backend module for pepper package

%description tdm
This package includes tdm backend module files.

###### tdm backend devel
%package tdm-devel
Summary: TDM backend development module for pepper package
Requires: pepper-tdm = %{version}-%{release}

%description tdm-devel
This package includes drm backend development module files.

###### fbdev backend
%package fbdev
Summary: Fbdev backend module for pepper package

%description fbdev
This package includes fbdev backend module files.

###### fbdev backend devel
%package fbdev-devel
Summary: Fbdev backend development module for pepper package
Requires: pepper-fbdev = %{version}-%{release}

%description fbdev-devel
This package includes fbdev backend development module files.

###### wayland backend
%package wayland
Summary: Wayland backend module for pepper package

%description wayland
This package includes wayland backend module files.

###### wayland backend devel
%package wayland-devel
Summary: Wayland backend development module for pepper package
Requires: pepper-wayland = %{version}-%{release}

%description wayland-devel
This package includes wayland backend development module files.

###### doctor server
%package doctor
Summary: Doctor server for pepper package
Requires: pepper pepper-keyrouter pepper-evdev

%description doctor
This package includes doctor server files.

###### samples
%package samples
Summary: samples for pepper package
Requires: pepper-drm pepper-desktop-shell
Requires: pepper-fbdev
Requires: pepper-tdm
Requires: pepper-wayland pepper-x11
Requires: pepper-libinput
Requires: pepper-keyrouter pepper-evdev
Requires: pepper-xkb

%description samples
This package includes samples files.

###### executing

%prep
%setup -q
cp %{SOURCE1001} .

%build
%autogen \
	--disable-x11 \
%if "%{ENABLE_TDM}" == "0"
	--disable-tdm \
%endif
	--enable-socket-fd=yes \
	--disable-document

make %{?_smp_mflags}

%install
%make_install

%define display_user display
%define display_group display

# install system session services
%__mkdir_p %{buildroot}%{_unitdir}
install -m 644 data/doctor/units/display-manager.service %{buildroot}%{_unitdir}
install -m 644 data/doctor/units/display-manager-ready.path %{buildroot}%{_unitdir}
install -m 644 data/doctor/units/display-manager-ready.service %{buildroot}%{_unitdir}

# install user session service
%__mkdir_p %{buildroot}%{_unitdir_user}
install -m 644 data/doctor/units/display-user.service %{buildroot}%{_unitdir_user}

# install env file and scripts for service
%__mkdir_p %{buildroot}%{_sysconfdir}/sysconfig
install -m 0644 data/doctor/units/display-manager.env %{buildroot}%{_sysconfdir}/sysconfig
%__mkdir_p %{buildroot}%{_sysconfdir}/profile.d
install -m 0644 data/doctor/units/display_env.sh %{buildroot}%{_sysconfdir}/profile.d

%post -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%post keyrouter -p /sbin/ldconfig
%postun keyrouter -p /sbin/ldconfig

%post evdev -p /sbin/ldconfig
%postun evdev -p /sbin/ldconfig

%post libinput -p /sbin/ldconfig
%postun libinput -p /sbin/ldconfig

%post desktop-shell -p /sbin/ldconfig
%postun desktop-shell -p /sbin/ldconfig

%post render -p /sbin/ldconfig
%postun render -p /sbin/ldconfig

%post drm -p /sbin/ldconfig
%postun drm -p /sbin/ldconfig

%post fbdev -p /sbin/ldconfig
%postun fbdev -p /sbin/ldconfig

%post wayland -p /sbin/ldconfig
%postun wayland -p /sbin/ldconfig

%pre doctor
# create groups 'display'
getent group %{display_group} >/dev/null || %{_sbindir}/groupadd -r -o %{display_group}
# create user 'display'
getent passwd %{display_user} >/dev/null || %{_sbindir}/useradd -r -g %{display_group} -d /run/display -s /bin/false -c "Display" %{display_user}

# create links within systemd's target(s)
%__mkdir_p %{_unitdir}/graphical.target.wants/
%__mkdir_p %{_unitdir_user}/basic.target.wants/
ln -sf ../display-manager.service %{_unitdir}/graphical.target.wants/
ln -sf ../display-manager-ready.service %{_unitdir}/graphical.target.wants/
ln -sf ../display-user.service %{_unitdir_user}/basic.target.wants/

%postun doctor
rm -f %{_unitdir}/graphical.target.wants/display-manager.service
rm -f %{_unitdir}/graphical.target.wants/display-manager-ready.service
rm -f %{_unitdir_user}/basic.target.wants/display-user.service

%files -n %{name}
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper.so.*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper.h
%{_includedir}/pepper/pepper-utils.h
%{_includedir}/pepper/pepper-utils-pixman.h
%{_includedir}/pepper/pepper-output-backend.h
%{_includedir}/pepper/pepper-input-backend.h
%{_libdir}/pkgconfig/pepper.pc
%{_libdir}/libpepper.so

%files keyrouter
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-keyrouter.so.*

%files keyrouter-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/keyrouter.h
%{_includedir}/pepper/pepper-keyrouter.h
%{_libdir}/pkgconfig/pepper-keyrouter.pc
%{_libdir}/libpepper-keyrouter.so

%files evdev
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-evdev.so.*

%files evdev-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-evdev.h
%{_libdir}/pkgconfig/pepper-evdev.pc
%{_libdir}/libpepper-evdev.so

%files xkb
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-xkb.so.*

%files xkb-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-xkb.h
%{_libdir}/pkgconfig/pepper-xkb.pc
%{_libdir}/libpepper-xkb.so

%files libinput
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-libinput.so.*

%files libinput-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-libinput.h
%{_libdir}/pkgconfig/pepper-libinput.pc
%{_libdir}/libpepper-libinput.so

%files desktop-shell
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-desktop-shell.so.*
%{_bindir}/shell-client

%files desktop-shell-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-desktop-shell.h
%{_includedir}/pepper/pepper-shell-client-protocol.h
%{_includedir}/pepper/xdg-shell-client-protocol.h
%{_libdir}/pkgconfig/pepper-desktop-shell.pc
%{_libdir}/libpepper-desktop-shell.so

%files render
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-render.so.*

%files render-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-render.h
%{_includedir}/pepper/pepper-*-renderer.h
%{_libdir}/pkgconfig/pepper-render.pc
%{_libdir}/libpepper-render.so

%files drm
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-drm.so.*

%files drm-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-drm.h
%{_libdir}/pkgconfig/pepper-drm.pc
%{_libdir}/libpepper-drm.so

%if "%{ENABLE_TDM}" == "1"
%files tdm
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-tdm.so.*

%files tdm-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-tdm.h
%{_libdir}/pkgconfig/pepper-tdm.pc
%{_libdir}/libpepper-tdm.so
%endif

%files fbdev
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-fbdev.so.*

%files fbdev-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-fbdev.h
%{_libdir}/pkgconfig/pepper-fbdev.pc
%{_libdir}/libpepper-fbdev.so

%files wayland
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_libdir}/libpepper-wayland.so.*

%files wayland-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-wayland.h
%{_libdir}/pkgconfig/pepper-wayland.pc
%{_libdir}/libpepper-wayland.so

%files doctor
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING
%{_bindir}/doctor*
%{_unitdir}/display-manager-ready.path
%{_unitdir}/display-manager-ready.service
%{_unitdir}//display-manager.service
%{_unitdir_user}/display-user.service
%config %{_sysconfdir}/sysconfig/display-manager.env
%config %{_sysconfdir}/profile.d/display_env.sh

%files samples
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/*-backend
%{_bindir}/sample-*
