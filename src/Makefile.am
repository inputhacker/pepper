SUBDIRS = lib/pepper        \
          lib/keyrouter      \
          lib/devicemgr     \
          lib/evdev         \
          lib/libinput      \
          lib/xkb           \
          lib/desktop-shell \
          lib/render        \
          lib/drm           \
          lib/fbdev         \
          lib/wayland       \
          lib/inotify

if ENABLE_TDM
SUBDIRS += lib/tdm
endif

if ENABLE_X11
SUBDIRS += lib/x11
endif

SUBDIRS += samples          \
           bin/doctor
