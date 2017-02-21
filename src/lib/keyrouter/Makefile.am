lib_LTLIBRARIES = libpepper-keyrouter.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_keyrouter_includedir=$(includedir)/pepper
libpepper_keyrouter_include_HEADERS = pepper-keyrouter.h pepper-keyrouter-wayland.h

libpepper_keyrouter_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_KEYROUTER_CFLAGS)
libpepper_keyrouter_la_LIBADD = $(PEPPER_KEYROUTER_LIBS)

libpepper_keyrouter_la_SOURCES = pepper-keyrouter-internal.h \
                                pepper-keyrouter.c \
                                pepper-keyrouter-wayland.c
