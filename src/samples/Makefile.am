bin_PROGRAMS =

# drm-backend
if ENABLE_DRM
bin_PROGRAMS += drm-backend

drm_backend_CFLAGS = $(SAMPLES_CFLAGS)
drm_backend_LDADD  = $(SAMPLES_LIBS)

drm_backend_SOURCES = drm-backend.c
endif

# tdm-backend
if ENABLE_TDM
bin_PROGRAMS += tdm-backend

tdm_backend_CFLAGS = $(SAMPLES_CFLAGS)
tdm_backend_LDADD  = $(SAMPLES_LIBS)

tdm_backend_SOURCES = tdm-backend.c
endif

# fbdev-backend
bin_PROGRAMS += fbdev-backend

fbdev_backend_CFLAGS = $(SAMPLES_CFLAGS)
fbdev_backend_LDADD  = $(SAMPLES_LIBS)

fbdev_backend_SOURCES = fbdev-backend.c

# wayland-backend
bin_PROGRAMS += wayland-backend

wayland_backend_CFLAGS = $(SAMPLES_CFLAGS)
wayland_backend_LDADD  = $(SAMPLES_LIBS)

wayland_backend_SOURCES = wayland-backend.c

# x11-backend
if ENABLE_X11
bin_PROGRAMS += x11-backend

x11_backend_CFLAGS = $(SAMPLES_CFLAGS)
x11_backend_LDADD  = $(SAMPLES_LIBS)

x11_backend_SOURCES = x11-backend.c
endif

bin_PROGRAMS += sample-server sample-client

sample_server_CFLAGS = $(SAMPLE_SERVER_CFLAGS)
sample_server_LDADD = $(SAMPLE_SERVER_LIBS)

sample_server_SOURCES = sample-server.c

sample_client_CFLAGS = $(SAMPLE_CLIENT_CFLAGS)
sample_client_LDADD = $(SAMPLE_CLIENT_LIBS)

sample_client_SOURCES = sample-client.c xdg-shell-client-protocol.h xdg-shell-protocol.c
