SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA =

pkgconfig_DATA += pkgconfig/pepper.pc
pkgconfig_DATA += pkgconfig/pepper-render.pc
if ENABLE_DRM
pkgconfig_DATA += pkgconfig/pepper-drm.pc
endif
if ENABLE_TDM
pkgconfig_DATA += pkgconfig/pepper-tdm.pc
endif
pkgconfig_DATA += pkgconfig/pepper-fbdev.pc
pkgconfig_DATA += pkgconfig/pepper-inotify.pc
pkgconfig_DATA += pkgconfig/pepper-keyrouter.pc
pkgconfig_DATA += pkgconfig/pepper-devicemgr.pc
pkgconfig_DATA += pkgconfig/pepper-evdev.pc
pkgconfig_DATA += pkgconfig/pepper-libinput.pc
pkgconfig_DATA += pkgconfig/pepper-xkb.pc
pkgconfig_DATA += pkgconfig/pepper-wayland.pc
if ENABLE_X11
pkgconfig_DATA += pkgconfig/pepper-x11.pc
endif
pkgconfig_DATA += pkgconfig/pepper-desktop-shell.pc

if ENABLE_DOCUMENT
SUBDIRS += doc
endif
