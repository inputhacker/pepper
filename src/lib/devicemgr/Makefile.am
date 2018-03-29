lib_LTLIBRARIES = libpepper-devicemgr.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_devicemgr_includedir=$(includedir)/pepper
libpepper_devicemgr_include_HEADERS = devicemgr.h pepper-devicemgr.h

libpepper_devicemgr_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_DEVICEMGR_CFLAGS)
libpepper_devicemgr_la_LIBADD = $(PEPPER_DEVICEMGR_LIBS)

libpepper_devicemgr_la_SOURCES = devicemgr-internal.h \
                                 devicemgr.c \
                                 pepper-devicemgr.c
