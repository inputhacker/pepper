lib_LTLIBRARIES = libpepper-inotify.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_inotify_includedir=$(includedir)/pepper
libpepper_inotify_include_HEADERS = pepper-inotify.h

libpepper_inotify_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_INOTIFY_CFLAGS)
libpepper_inotify_la_LIBADD = $(PEPPER_INOTIFY_LIBS)

libpepper_inotify_la_SOURCES = pepper-inotify.c
