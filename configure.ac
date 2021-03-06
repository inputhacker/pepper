m4_define([pepper_major], 0)
m4_define([pepper_minor], 0)
m4_define([pepper_micro], 0)

m4_define([pepper_version], [pepper_major.pepper_minor.pepper_micro])

AC_PREREQ([2.64])
AC_INIT([pepper], [pepper_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_VERSION_MAJOR], [pepper_major_version])
AC_SUBST([PEPPER_VERSION_MINOR], [pepper_minor_version])
AC_SUBST([PEPPER_VERSION_MICRO], [pepper_micro_version])
AC_SUBST([PEPPER_VERSION], [pepper_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

if test "x$GCC" = "xyes"; then
GCC_CFLAGS="-Wall -Wextra -Wno-unused-parameter \
            -Wno-missing-field-initializers -g -fvisibility=hidden \
            -Wstrict-prototypes -Wmissing-prototypes -Wsign-compare"
fi
AC_SUBST(GCC_CFLAGS)

# pepper
PEPPER_REQUIRES="wayland-server"
PKG_CHECK_MODULES(PEPPER, [$PEPPER_REQUIRES])
PKG_CHECK_MODULES([TBM], [libtbm wayland-tbm-server], [have_tbm=yes], [have_tbm=no])
if test x$have_tbm = xyes; then
   AC_DEFINE([HAVE_TBM], [1], [Build the tbm backend])
   PEPPER_REQUIRES="$PEPPER_REQUIRES libtbm wayland-tbm-server"
fi
AM_CONDITIONAL([HAVE_TBM], [test "x${have_tbm}" = "xyes"])

PKG_CHECK_MODULES(DLOG, dlog, [have_dlog="yes"], [have_dlog="no"])
if test "x${have_dlog}" = "xyes"; then
	AC_DEFINE([HAVE_DLOG], [1], [Define to 1 if you have dlog])
	PEPPER_REQUIRES="$PEPPER_REQUIRES dlog"
fi
AM_CONDITIONAL([HAVE_DLOG], [test "x${have_dlog}" = "xyes"])

PKG_CHECK_MODULES(CYNARA, [cynara-client, cynara-creds-socket, cynara-session, libsmack],
   [have_cynara="yes"], [have_cynara="no"])
if test "x${have_cynara}" = "xyes"; then
	AC_DEFINE([HAVE_CYNARA], [1], [Define to 1 if you have cynara])
	PEPPER_REQUIRES="$PEPPER_REQUIRES cynara-client cynara-creds-socket cynara-session libsmack"
	PEPPER_CFLAGS+="$CYNARA_CFLAGS -DHAVE_CYNARA=1 "
	PEPPER_LIBS+="$CYNARA_LIBS "
fi
AM_CONDITIONAL([HAVE_CYNARA], [test "x${have_cynara}" = "xyes"])

PEPPER_DIR="-I\$(top_srcdir)/src/lib/pepper"
PEPPER_LIB="\$(top_srcdir)/src/lib/pepper/libpepper.la"

AC_SUBST(PEPPER_REQUIRES)

AC_ARG_ENABLE(socket_fd,
              AC_HELP_STRING([--enable-socket-fd],
                             [enable wl_display_add_socket_fd()]),
              enable_socket_fd=$enableval,
              enable_socket_fd=no)

if test x$enable_socket_fd = xyes; then
    AC_DEFINE([ENABLE_SOCKET_FD], [1], [Use wl_display_add_socket_fd])
fi

# pepper-inotify
PEPPER_INOTIFY_REQUIRES="pepper"

PEPPER_INOTIFY_DIR="-I\$(top_srcdir)/src/lib/inotify"
PEPPER_INOTIFY_LIB="\$(top_srcdir)/src/lib/inotify/libpepper-inotify.la"

PEPPER_INOTIFY_CFLAGS="$PEPPER_DIR $PEPPER_INOTIFY_DIR $PEPPER_CFLAGS $PEPPER_INOTIFY_CFLAGS "
PEPPER_INOTIFY_LIBS="$PEPPER_LIB $PEPPER_INOTIFY_LIBS "

AC_SUBST(PEPPER_INOTIFY_CFLAGS)
AC_SUBST(PEPPER_INOTIFY_LIBS)
AC_SUBST(PEPPER_INOTIFY_REQUIRES)

# pepper-keyrouter
PEPPER_KEYROUTER_REQUIRES="tizen-extension-server"
PKG_CHECK_MODULES(PEPPER_KEYROUTER, [$PEPPER_KEYROUTER_REQUIRES])

AC_ARG_WITH(keylayout-dir,
            [AS_HELP_STRING([--with-keylayout-dir=dir],
            [ tizen keylayout dir ])],
            KEYLAYOUT_DIR="$withval")
AC_DEFINE_UNQUOTED([KEYLAYOUT_DIR], ["$KEYLAYOUT_DIR"], [Keylayout DIR])

PEPPER_KEYROUTER_DIR="-I\$(top_srcdir)/src/lib/keyrouter"
PEPPER_KEYROUTER_LIB="\$(top_srcdir)/src/lib/keyrouter/libpepper-keyrouter.la"

PEPPER_KEYROUTER_CFLAGS="$PEPPER_DIR $PEPPER_KEYROUTER_CFLAGS $PEPPER_CFLAGS $PEPPER_KEYROUTER_DIR"
PEPPER_KEYROUTER_LIBS="$PEPPER_LIB $PEPPER_KEYROUTER_LIBS"

AC_SUBST(PEPPER_KEYROUTER_CFLAGS)
AC_SUBST(PEPPER_KEYROUTER_LIBS)
AC_SUBST(PEPPER_KEYROUTER_REQUIRES)

# pepper-libinput
PEPPER_LIBINPUT_REQUIRES="libinput"
PKG_CHECK_MODULES(PEPPER_LIBINPUT, [$PEPPER_LIBINPUT_REQUIRES])
#PEPPER_LIBINPUT_REQUIRES="$PEPPER_LIBINPUT_REQUIRES pepper"

PEPPER_LIBINPUT_DIR="-I\$(top_srcdir)/src/lib/libinput"
PEPPER_LIBINPUT_LIB="\$(top_srcdir)/src/lib/libinput/libpepper-libinput.la"

PEPPER_LIBINPUT_CFLAGS="$PEPPER_DIR $PEPPER_LIBINPUT_CFLAGS $PEPPER_CFLAGS"
PEPPER_LIBINPUT_LIBS="$PEPPER_LIB $PEPPER_LIBINPUT_LIBS"

AC_SUBST(PEPPER_LIBINPUT_CFLAGS)
AC_SUBST(PEPPER_LIBINPUT_LIBS)
AC_SUBST(PEPPER_LIBINPUT_REQUIRES)

# pepper-xkb
PEPPER_XKB_REQUIRES="xkbcommon"
PKG_CHECK_MODULES(PEPPER_XKB, [$PEPPER_XKB_REQUIRES])

PEPPER_XKB_DIR="-I\$(top_srcdir)/src/lib/xkb"
PEPPER_XKB_LIB="\$(top_srcdir)/src/lib/xkb/libpepper-xkb.la"

PEPPER_XKB_CFLAGS="$PEPPER_DIR $PEPPER_XKB_DIR $PEPPER_CFLAGS $PEPPER_XKB_CFLAGS"
PEPPER_XKB_LIBS="$PEPPER_LIB $PEPPER_XKB_LIBS"

AC_SUBST(PEPPER_XKB_CFLAGS)
AC_SUBST(PEPPER_XKB_LIBS)
AC_SUBST(PEPPER_XKB_REQUIRES)

# pepper-devicemgr
PEPPER_DEVICEMGR_REQUIRES="tizen-extension-server"
PKG_CHECK_MODULES(PEPPER_DEVICEMGR, [$PEPPER_DEVICEMGR_REQUIRES])

PEPPER_DEVICEMGR_DIR="-I\$(top_srcdir)/src/lib/devicemgr"
PEPPER_DEVICEMGR_LIB="\$(top_srcdir)/src/lib/devicemgr/libpepper-devicemgr.la"

PEPPER_DEVICEMGR_CFLAGS="$PEPPER_DIR $PEPPER_DEVICEMGR_CFLAGS $PEPPER_CFLAGS $PEPPER_DEVICEMGR_DIR $PEPPER_XKB_CFLAGS "
PEPPER_DEVICEMGR_LIBS="$PEPPER_LIB $PEPPER_DEVICEMGR_LIBS $PEPPER_XKB_LIBS "

AC_SUBST(PEPPER_DEVICEMGR_CFLAGS)
AC_SUBST(PEPPER_DEVICEMGR_LIBS)
AC_SUBST(PEPPER_DEVICEMGR_REQUIRES)

# pepper-desktop-shell
PEPPER_DESKTOP_SHELL_REQUIRES="pepper"

PEPPER_DESKTOP_SHELL_DIR="-I\$(top_srcdir)/src/lib/desktop-shell"
PEPPER_DESKTOP_SHELL_LIB="\$(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la"

PEPPER_DESKTOP_SHELL_CFLAGS="$PEPPER_DIR $PEPPER_CFLAGS"
PEPPER_DESKTOP_SHELL_CFLAGS="$PEPPER_XKB_DIR $PEPPER_DESKTOP_SHELL_CFLAGS"
PEPPER_DESKTOP_SHELL_LIBS="$PEPPER_LIB"
PEPPER_DESKTOP_SHELL_LIBS="$PEPPER_XKB_LIB $PEPPER_XKB_LIBS $PEPPER_DESKTOP_SHELL_LIBS"

AC_SUBST(PEPPER_DESKTOP_SHELL_CFLAGS)
AC_SUBST(PEPPER_DESKTOP_SHELL_LIBS)
AC_SUBST(PEPPER_DESKTOP_SHELL_REQUIRES)

# pepper-evdev
PEPPER_EVDEV_REQUIRES="pepper"

PEPPER_EVDEV_DIR="-I\$(top_srcdir)/src/lib/evdev"
PEPPER_EVDEV_LIB="\$(top_srcdir)/src/lib/evdev/libpepper-evdev.la"

PEPPER_EVDEV_CFLAGS="$PEPPER_DIR $PEPPER_EVDEV_DIR $PEPPER_CFLAGS $PEPPER_EVDEV_CFLAGS"
PEPPER_EVDEV_LIBS="$PEPPER_LIB $PEPPER_EVDEV_LIBS"

AC_SUBST(PEPPER_EVDEV_CFLAGS)
AC_SUBST(PEPPER_EVDEV_LIBS)
AC_SUBST(PEPPER_EVDEV_REQUIRES)

# pepper-render
PEPPER_RENDER_REQUIRES="pixman-1 egl glesv2"
PKG_CHECK_MODULES(PEPPER_RENDER, [$PEPPER_RENDER_REQUIRES])

PEPPER_RENDER_DIR="-I\$(top_srcdir)/src/lib/render"
PEPPER_RENDER_LIB="\$(top_srcdir)/src/lib/render/libpepper-render.la"

PEPPER_RENDER_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_CFLAGS $PEPPER_CFLAGS"
PEPPER_RENDER_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIBS"

AC_SUBST(PEPPER_RENDER_CFLAGS)
AC_SUBST(PEPPER_RENDER_LIBS)
AC_SUBST(PEPPER_RENDER_REQUIRES)

# pepper-drm
AC_ARG_ENABLE(drm,
              AC_HELP_STRING([--disable-drm],
                             [disable drm backend module]),
              enable_drm=$enableval,
              enable_drm=yes)

AM_CONDITIONAL(ENABLE_DRM, test x$enable_drm = xyes)

if test x$enable_drm = xyes; then
	PEPPER_DRM_REQUIRES="libdrm gbm pixman-1"
	PKG_CHECK_MODULES(PEPPER_DRM, [$PEPPER_DRM_REQUIRES])

	PKG_CHECK_MODULES([DRM_SPRD], [libdrm_sprd], [have_drm_sprd=yes], [have_drm_sprd=no])
	if test x$have_drm_sprd = xyes; then
	   AC_DEFINE([HAVE_DRM_SPRD], [1], [Build the drm_sprd])
	   PEPPER_DRM_REQUIRES="$PEPPER_DRM_REQUIRES libdrm-sprd"
	   PEPPER_DRM_CFLAGS="$PEPPER_DRM_CFLAGS $DRM_SPRD_CFLAGS"
	   PEPPER_DRM_LIBS="$PEPPER_DRM_LIBS $DRM_SPRD_LIBS"
	fi

	PEPPER_DRM_DIR="-I\$(top_srcdir)/src/lib/drm"
	PEPPER_DRM_LIB="\$(top_srcdir)/src/lib/drm/libpepper-drm.la"

	PEPPER_DRM_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_LIBINPUT_DIR $PEPPER_DRM_CFLAGS $TBM_CFLAGS"
	PEPPER_DRM_CFLAGS="$PEPPER_DRM_CFLAGS $PEPPER_CFLAGS"
	PEPPER_DRM_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_LIBINPUT_LIB $PEPPER_DRM_LIBS $TBM_LIBS"

	AC_SUBST(PEPPER_DRM_CFLAGS)
	AC_SUBST(PEPPER_DRM_LIBS)
	AC_SUBST(PEPPER_DRM_REQUIRES)
fi

# pepper-tdm
AC_ARG_ENABLE(tdm,
              AC_HELP_STRING([--disable-tdm],
                             [disable tdm backend module]),
              enable_tdm=$enableval,
              enable_tdm=yes)

AM_CONDITIONAL(ENABLE_TDM, test x$enable_tdm = xyes)

if test x$enable_tdm = xyes; then
	PEPPER_TDM_REQUIRES="libtbm libtdm pixman-1"
	PKG_CHECK_MODULES(PEPPER_TDM, [$PEPPER_TDM_REQUIRES])
	#PEPPER_TDM_REQUIRES="$PEPPER_TDM_REQUIRES pepper pepper-render pepper-libinput"

	PEPPER_TDM_DIR="-I\$(top_srcdir)/src/lib/tdm"
	PEPPER_TDM_LIB="\$(top_srcdir)/src/lib/tdm/libpepper-tdm.la"

	PEPPER_TDM_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_LIBINPUT_DIR $PEPPER_TDM_CFLAGS"
	PEPPER_TDM_CFLAGS="$PEPPER_TDM_CFLAGS $PEPPER_CFLAGS"
	PEPPER_TDM_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_LIBINPUT_LIB $PEPPER_TDM_LIBS"

	AC_SUBST(PEPPER_TDM_CFLAGS)
	AC_SUBST(PEPPER_TDM_LIBS)
	AC_SUBST(PEPPER_TDM_REQUIRES)
fi

# pepper-fbdev
PEPPER_FBDEV_REQUIRES+="pixman-1"
PKG_CHECK_MODULES(PEPPER_FBDEV, [$PEPPER_FBDEV_REQUIRES])

PEPPER_FBDEV_DIR="-I\$(top_srcdir)/src/lib/fbdev"
PEPPER_FBDEV_LIB="\$(top_srcdir)/src/lib/fbdev/libpepper-fbdev.la"

PEPPER_FBDEV_CFLAGS+="$PEPPER_CFLAGS $PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_LIBINPUT_DIR"
PEPPER_FBDEV_LIBS+="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_LIBINPUT_LIB"

AC_SUBST(PEPPER_FBDEV_CFLAGS)
AC_SUBST(PEPPER_FBDEV_LIBS)
AC_SUBST(PEPPER_FBDEV_REQUIRES)

# pepper-wayland
PEPPER_WAYLAND_REQUIRES="wayland-client pixman-1"
PKG_CHECK_MODULES(PEPPER_WAYLAND, [$PEPPER_WAYLAND_REQUIRES])

PEPPER_WAYLAND_DIR="-I\$(top_srcdir)/src/lib/wayland"
PEPPER_WAYLAND_LIB="\$(top_srcdir)/src/lib/wayland/libpepper-wayland.la"

PEPPER_WAYLAND_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_WAYLAND_CFLAGS $PEPPER_CFLAGS"
PEPPER_WAYLAND_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_WAYLAND_LIBS"

AC_SUBST(PEPPER_WAYLAND_CFLAGS)
AC_SUBST(PEPPER_WAYLAND_LIBS)
AC_SUBST(PEPPER_WAYLAND_REQUIRES)

# pepper-x11
AC_ARG_ENABLE(x11,
              AC_HELP_STRING([--disable-x11],
                             [disable x11 backend module]),
              enable_x11=$enableval,
              enable_x11=yes)

AM_CONDITIONAL(ENABLE_X11, test x$enable_x11 = xyes)

if test x$enable_x11 = xyes; then
    PEPPER_X11_REQUIRES="x11 xcb-shm x11-xcb pixman-1"
    PKG_CHECK_MODULES(PEPPER_X11, [$PEPPER_X11_REQUIRES])

    PEPPER_X11_DIR="-I\$(top_srcdir)/src/lib/x11"
    PEPPER_X11_LIB="\$(top_srcdir)/src/lib/x11/libpepper-x11.la"

    PEPPER_X11_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_X11_CFLAGS $PEPPER_CFLAGS"
    PEPPER_X11_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_X11_LIBS"

    AC_SUBST(PEPPER_X11_CFLAGS)
    AC_SUBST(PEPPER_X11_LIBS)
    AC_SUBST(PEPPER_X11_REQUIRES)
fi

# shell client
SHELL_CLIENT_REQUIRES="wayland-client pixman-1"
PKG_CHECK_MODULES(SHELL_CLIENT, [$SHELL_CLIENT_REQUIRES])

SHELL_CLIENT_CFLAGS="$PEPPER_DIR $SHELL_CLIENT_CFLAGS"
SHELL_CLIENT_LIBS="$PEPPER_LIB $PEPPER_LIBS $SHELL_CLIENT_LIBS"

AC_SUBST(SHELL_CLIENT_CFLAGS)
AC_SUBST(SHELL_CLIENT_LIBS)

# sample
SAMPLES_REQUIRES="wayland-server wayland-client libudev"
PKG_CHECK_MODULES(SAMPLES, $[SAMPLES_REQUIRES])

SAMPLES_CFLAGS="$PEPPER_DIR $PEPPER_DESKTOP_SHELL_DIR $PEPPER_RENDER_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$PEPPER_LIBINPUT_DIR $PEPPER_DRM_DIR $PEPPER_FBDEV_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$PEPPER_WAYLAND_DIR $PEPPER_X11_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$PEPPER_TDM_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$SAMPLES_CFLAGS $PEPPER_CFLAGS"

SAMPLES_LIBS="$PEPPER_LIB $PEPPER_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_DESKTOP_SHELL_LIB $PEPPER_DESKTOP_SHELL_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_RENDER_LIB $PEPPER_RENDER_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_LIBINPUT_LIB $PEPPER_LIBINPUT_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_DRM_LIB $PEPPER_DRM_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_TDM_LIB $PEPPER_TDM_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_FBDEV_LIB $PEPPER_FBDEV_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_WAYLAND_LIB $PEPPER_WAYLAND_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_X11_LIB $PEPPER_X11_LIBS $SAMPLES_LIBS"

AC_SUBST(SAMPLES_CFLAGS)
AC_SUBST(SAMPLES_LIBS)

# doctor (headless server) and headless client samples
DOCTOR_SERVER_REQUIRES="wayland-server libtbm"
PKG_CHECK_MODULES(DOCTOR_SERVER, $[DOCTOR_SERVER_REQUIRES])
DOCTOR_SERVER_CFLAGS="$PEPPER_DIR $PEPPER_EVDEV_DIR $PEPPER_XKB_DIR $PEPPER_KEYROUTER_DIR $PEPPER_DEVICEMGR_DIR $DOCTOR_SERVER_CFLAGS"
DOCTOR_SERVER_LIBS="$PEPPER_LIB $PEPPER_LIBS $DOCTOR_SERVER_LIBS"
DOCTOR_SERVER_LIBS="$PEPPER_EVDEV_LIB $PEPPER_EVDEV_LIBS $DOCTOR_SERVER_LIBS"
DOCTOR_SERVER_LIBS="$PEPPER_XKB_LIB $PEPPER_XKB_LIBS $DOCTOR_SERVER_LIBS"
DOCTOR_SERVER_LIBS="$PEPPER_KEYROUTER_LIB $PEPPER_KEYROUTER_LIBS $PEPPER_DEVICEMGR_LIB $PEPPER_DEVICEMGR_LIBS $DOCTOR_SERVER_LIBS"

AC_SUBST(DOCTOR_SERVER_CFLAGS)
AC_SUBST(DOCTOR_SERVER_LIBS)

DOCTOR_CLIENT_REQUIRES="wayland-client tizen-extension-client"
PKG_CHECK_MODULES(DOCTOR_CLIENT, $[DOCTOR_CLIENT_REQUIRES])
DOCTOR_CLIENT_CFLAGS="$DOCTOR_CLIENT_CFLAGS"
DOCTOR_CLIENT_LIBS="$DOCTOR_CLIENT_LIBS"

AC_SUBST(DOCTOR_CLIENT_CFLAGS)
AC_SUBST(DOCTOR_CLIENT_LIBS)

# sample client
SAMPLE_CLIENT_REQUIRES="wayland-client tizen-extension-client xkbcommon"
PKG_CHECK_MODULES(SAMPLE_CLIENT, $[SAMPLE_CLIENT_REQUIRES])
SAMPLE_CLIENT_CFLAGS="$SAMPLE_CLIENT_CFLAGS"
SAMPLE_CLIENT_LIBS="$SAMPLE_CLIENT_LIBS"

AC_SUBST(SAMPLE_CLIENT_CFLAGS)
AC_SUBST(SAMPLE_CLIENT_LIBS)

# sample server
SAMPLE_SERVER_REQUIRES="wayland-server xkbcommon"
PKG_CHECK_MODULES(SAMPLE_SERVER, [$SAMPLE_SERVER_REQUIRES])

SAMPLE_SERVER_CFLAGS="$PEPPER_DIR $SAMPLE_SERVER_CFLAGS"
SAMPLE_SERVER_CFLAGS="$PEPPER_EVDEV_DIR $PEPPER_KEYROUTER_DIR $PEPPER_DEVICEMGR_DIR $SAMPLE_SERVER_CFLAGS"
SAMPLE_SERVER_CFLAGS="$PEPPER_INOTIFY_DIR $SAMPLE_SERVER_CFLAGS"
SAMPLE_SERVER_CFLAGS="$PEPPER_XKB_DIR $SAMPLE_SERVER_CFLAGS"

SAMPLE_SERVER_LIBS="$PEPPER_LIB $SAMPLE_SERVER_LIBS"
SAMPLE_SERVER_LIBS="$PEPPER_EVDEV_LIB $PEPPER_EVDEV_LIBS $SAMPLE_SERVER_LIBS"
SAMPLE_SERVER_LIBS="$PEPPER_KEYROUTER_LIB $PEPPER_KEYROUTER_LIBS $PEPPER_DEVICEMGR_LIB $PEPPER_DEVICEMGR_LIBS $SAMPLE_SERVER_LIBS"
SAMPLE_SERVER_LIBS="$PEPPER_INOTIFY_LIB $PEPPER_INOTIFY_LIBS $SAMPLE_SERVER_LIBS"
SAMPLE_SERVER_LIBS="$PEPPER_XKB_LIB $PEPPER_XKB_LIBS $SAMPLE_SERVER_LIBS"

AC_SUBST(SAMPLE_SERVER_CFLAGS)
AC_SUBST(SAMPLE_SERVER_LIBS)

# wayland-scanner
AC_PATH_PROG([wayland_scanner], [wayland-scanner])
if test x$wayland_scanner = x; then
    PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
    wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
fi

# document
AC_ARG_ENABLE([document],
              [AC_HELP_STRING([--disable-document],
                              [Disable building the documentation])],
              [],
              [enable_document=yes])

AM_CONDITIONAL(ENABLE_DOCUMENT, test x$enable_document = xyes)

if test x$enable_document = xyes; then
    AC_PATH_PROG(DOXYGEN, [doxygen])
    if test x$DOXYGEN = x; then
        AC_MSG_ERROR([Documentation need doxygen, but not found])
    fi
fi

# Output files
AC_CONFIG_FILES([
Makefile
src/Makefile
src/lib/pepper/Makefile
src/lib/inotify/Makefile
src/lib/keyrouter/Makefile
src/lib/devicemgr/Makefile
src/lib/evdev/Makefile
src/lib/xkb/Makefile
src/lib/libinput/Makefile
src/lib/desktop-shell/Makefile
src/lib/render/Makefile
src/lib/fbdev/Makefile
src/lib/wayland/Makefile
src/bin/doctor/Makefile
src/samples/Makefile
pkgconfig/pepper.pc
pkgconfig/pepper-render.pc
pkgconfig/pepper-fbdev.pc
pkgconfig/pepper-inotify.pc
pkgconfig/pepper-keyrouter.pc
pkgconfig/pepper-devicemgr.pc
pkgconfig/pepper-evdev.pc
pkgconfig/pepper-xkb.pc
pkgconfig/pepper-libinput.pc
pkgconfig/pepper-wayland.pc
pkgconfig/pepper-desktop-shell.pc
])

if test x$enable_tdm = xyes; then
    AC_CONFIG_FILES([
        src/lib/tdm/Makefile
        pkgconfig/pepper-tdm.pc
    ])
fi

if test x$enable_drm = xyes; then
    AC_CONFIG_FILES([
        src/lib/drm/Makefile
        pkgconfig/pepper-drm.pc
    ])
fi

if test x$enable_x11 = xyes; then
    AC_CONFIG_FILES([
        src/lib/x11/Makefile
        pkgconfig/pepper-x11.pc
    ])
fi

if test x$enable_document = xyes; then
    AC_CONFIG_FILES([
        doc/Makefile
        doc/doxygen/pepper.doxygen
    ])
fi

AC_OUTPUT
