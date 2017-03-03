bin_PROGRAMS = headless

headless_CFLAGS = $(HEADLESS_CFLAGS)
headless_LDADD = $(HEADLESS_LIBS)
headless_LDFLAGS = $(HEADLESS_LIBS)

headless_SOURCES = \
	server.c \
	server.h

bin_PROGRAMS += headless-client

headless_client_CFLAGS = $(HEADLESS_CLIENT_CFLAGS)
headless_client_LDADD = $(HEADLESS_CLIENT_LIBS)
headless_client_LDFLAGS = $(HEADLESS_CLIENT_LIBS)
headless_client_SOURCES = headless-client.c
