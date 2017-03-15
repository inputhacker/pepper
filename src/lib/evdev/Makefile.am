lib_LTLIBRARIES = libpepper-evdev.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_evdev_includedir=$(includedir)/pepper
libpepper_evdev_include_HEADERS = pepper-evdev.h

libpepper_evdev_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_EVDEV_CFLAGS)
libpepper_evdev_la_LIBADD = $(PEPPER_EVDEV_LIBS)

libpepper_evdev_la_SOURCES = evdev.c
