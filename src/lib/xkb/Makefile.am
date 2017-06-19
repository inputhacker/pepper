lib_LTLIBRARIES = libpepper-xkb.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_xkb_includedir=$(includedir)/pepper
libpepper_xkb_include_HEADERS = pepper-xkb.h

libpepper_xkb_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_XKB_CFLAGS)
libpepper_xkb_la_LIBADD = $(PEPPER_XKB_LIBS)

libpepper_xkb_la_SOURCES = xkb.c
