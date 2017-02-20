lib_LTLIBRARIES = libpepper-headless.la
#bin_PROGRAMS = shell-client
BUILT_SOURCES =
CLEANFILES =

AM_CFLAGS = $(GCC_CFLAGS)

BUILT_SOURCES += protocol/tizen-headless-protocol.c         \
	protocol/tizen-headless-server-protocol.h  \
	protocol/tizen-headless-client-protocol.h

libpepper_headless_includedir=$(includedir)/pepper
libpepper_headless_include_HEADERS = pepper-headless.h                    \
	protocol/tizen-headless-client-protocol.h

libpepper_headless_la_CFLAGS = $(AM_CFLAGS)                    \
	-I$(srcdir)/protocol/           \
	$(PEPPER_HEADLESS_CFLAGS)

libpepper_headless_la_LIBADD = $(PEPPER_HEADLESS_LIBS)

libpepper_headless_la_SOURCES = headless-internal.h       \
	pepper-headless.c	\
	$(BUILT_SOURCES)

CLEANFILES += $(BUILT_SOURCES)

$(srcdir)/protocol/%-protocol.c : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) code < $< > $@

$(srcdir)/protocol/%-server-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) server-header < $< > $@

$(srcdir)/protocol/%-client-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) client-header < $< > $@

#shell_client_CFLAGS = $(SHELL_CLIENT_CFLAGS) -I$(srcdir)/protocol/
#shell_client_LDADD = $(SHELL_CLIENT_LIBS) libpepper-headless-shell.la
#shell_client_SOURCES = shell-client.c
