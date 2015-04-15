#ifndef X11_INTERNAL_H
#define X11_INTERNAL_H

#include "pepper-x11.h"

#include <common.h>
#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <string.h>

#define X11_BACKEND_INPUT_ID 0x12345678

typedef struct x11_output   x11_output_t;
typedef struct x11_cursor   x11_cursor_t;
typedef struct x11_seat     x11_seat_t;

struct x11_output
{
    pepper_x11_connection_t *connection;

    int32_t             x, y;
    uint32_t            w, h;
    uint32_t            subpixel;
    uint32_t            scale;
    uint8_t             depth;

    xcb_window_t        window;
    xcb_gc_t            gc;
    x11_cursor_t        *cursor;

    struct wl_signal    destroy_signal;
    struct wl_signal    mode_change_signal;

    struct wl_listener  conn_destroy_listener;

    struct wl_list      link;
};

struct x11_seat
{
    pepper_seat_t       *base;

    uint32_t            id;
    uint32_t            caps;
    char                *name;

    wl_fixed_t          pointer_x_last;
    wl_fixed_t          pointer_y_last;
    wl_fixed_t          touch_x_last;   /* FIXME */
    wl_fixed_t          touch_y_last;   /* FIXME */

    struct wl_list      link;
    struct wl_signal    capabilities_signal;
    struct wl_signal    name_signal;
};

struct pepper_x11_connection
{
    pepper_compositor_t    *compositor;
    char                   *display_name;

    Display                *display;
    xcb_screen_t           *screen;
    xcb_connection_t       *xcb_connection;

    struct wl_event_source *xcb_event_source;
    struct wl_list          link;
    int fd;

    struct wl_list          outputs;

    pepper_bool_t           use_xinput;
    x11_seat_t              *seat;

    struct {
        xcb_atom_t          wm_protocols;
        xcb_atom_t          wm_normal_hints;
        xcb_atom_t          wm_size_hints;
        xcb_atom_t          wm_delete_window;
        xcb_atom_t          wm_class;
        xcb_atom_t          net_wm_name;
        xcb_atom_t          net_supporting_wm_check;
        xcb_atom_t          net_supported;
        xcb_atom_t          net_wm_icon;
        xcb_atom_t          net_wm_state;
        xcb_atom_t          net_wm_state_fullscreen;
        xcb_atom_t          string;
        xcb_atom_t          utf8_string;
        xcb_atom_t          cardinal;
        xcb_atom_t          xkb_names;
    } atom;

    struct wl_signal        destroy_signal;
};

struct x11_cursor
{
    xcb_cursor_t xcb_cursor;
    int w;
    int h;
    uint8_t *data;
};

/* it declared in xcb-icccm.h */
typedef struct xcb_size_hints {
    uint32_t flags;
    uint32_t pad[4];
    int32_t  min_width, min_height;
    int32_t  max_width, max_height;
    int32_t  width_inc, height_inc;
    int32_t  min_aspect_x, min_aspect_y;
    int32_t  max_aspect_x, max_aspect_y;
    int32_t  base_width, base_height;
    int32_t  win_gravity;
}xcb_size_hints_t;
#define WM_NORMAL_HINTS_MIN_SIZE        16
#define WM_NORMAL_HINTS_MAX_SIZE        32
/* -- xcb-icccm.h */

pepper_x11_connection_t *
pepper_x11_connect(pepper_compositor_t *compositor, const char *display_name);

void
x11_window_input_property_change(xcb_connection_t *conn, xcb_window_t window);

void
x11_handle_input_event(x11_seat_t* seat, uint32_t type, xcb_generic_event_t* xev);

#endif  /*X11_INTERNAL_H*/
