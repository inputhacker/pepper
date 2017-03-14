#ifndef PEPPER_EVDEV_INTERNAL_H
#define PEPPER_EVDEV_INTERNAL_H

#include <pepper.h>

typedef struct pepper_evdev pepper_evdev_t;
typedef struct evdev_device_info evdev_device_info_t;
typedef struct evdev_key_event evdev_key_event_t;

struct pepper_evdev
{
       pepper_compositor_t *compositor;
       struct wl_display *display;
       struct wl_event_loop *event_loop;
       pepper_list_t key_event_queue;
       pepper_list_t device_list;
};

struct evdev_device_info
{
       pepper_evdev_t *evdev;
       pepper_input_device_t *device;
       struct wl_event_source *event_source;
       pepper_list_t link;
};

struct evdev_key_event
{
       pepper_evdev_t *evdev;
       pepper_input_device_t *device;
       unsigned int keycode;
       unsigned int state;
       unsigned int time;
       pepper_list_t link;
};

#endif /* PEPPER_EVDEV_INTERNAL_H */
