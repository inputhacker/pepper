if [ "$USER" == "root" ]; then
        export XDG_RUNTIME_DIR=/run
else
        export XDG_RUNTIME_DIR=/run/user/$UID
fi
if [ "$WAYALND_DISPLAY" = "" ]; then
        export WAYLAND_DISPLAY=headless-0
fi
