/*
* Copyright © 2015-2017 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#ifndef PEPPER_EVDEV_INTERNAL_H
#define PEPPER_EVDEV_INTERNAL_H

#include <pepper.h>
#include <pepper-evdev.h>

#define MAX_PATH_LEN 16

#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#endif

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
       int fd;
       char path[MAX_PATH_LEN];
       unsigned int caps;
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
