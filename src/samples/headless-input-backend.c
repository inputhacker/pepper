/*
* Copyright © 2017 Samsung Electronics co., Ltd. All Rights Reserved.
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

//pepper_input_device_create

#include <pepper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#ifdef SIZE_EPOLL
# undef SIZE_EPOLL
#endif
#define SIZE_EPOLL 16

#ifdef SIZE_BUFFER
# undef SIZE_BUFFER
#endif
#define SIZE_BUFFER 64

PEPPER_API int headless_input_backend_init(void *);
PEPPER_API int headless_input_backend_fini(void *);

typedef struct _Input_Global_DB
{
	pepper_compositor_t *compositor;
	struct wl_display *display;
	struct wl_event_loop *event_loop;
	struct wl_event_source *event_source;

	pepper_bool_t init;
	int enable_log;
} Input_Global_DB;

Input_Global_DB input_db;

static void
event_process(struct input_event *ev)
{
	switch (ev->type) {
		case EV_KEY:
			PEPPER_TRACE("key event: code: %d, value: %d\n", ev->code, ev->value);
			break;
		case EV_SYN:
			PEPPER_TRACE("sync\n");
			break;
		default:
			break;
	}
}

static int
_headless_input_backend_event_loop(int fd, uint32_t mask, void *data)
{
	struct input_event iev[SIZE_BUFFER];
	int i, len;

	memset(iev, 0, sizeof(struct input_event)*SIZE_BUFFER);
	if ((len = read(fd,  &iev, sizeof(iev))) != -1) {
		for (i = 0; i < len / sizeof(iev[0]); i++) {
			event_process(&iev[i]);
		}
	}

	return 1;
}

static void
_headless_input_open_device(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) < 0) {
		PEPPER_ERROR("Failed to open %s\n", path);
		return;
	}
	input_db.event_source = wl_event_loop_add_fd(input_db.event_loop, fd, WL_EVENT_READABLE,
	                                             _headless_input_backend_event_loop, NULL);
}

PEPPER_API int
headless_input_backend_init(void *headless)
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	input_db.compositor = (pepper_compositor_t *)headless;
	PEPPER_CHECK(input_db.compositor, return 0, "Invalid compositor\n");
	input_db.display = pepper_compositor_get_display(input_db.compositor);
	PEPPER_CHECK(input_db.display, return 0, "Invalid wl_display\n");
	input_db.event_loop = wl_display_get_event_loop(input_db.display);
	PEPPER_CHECK(input_db.event_loop, return 0, "No event loop exist.\n");

	if (input_db.init == PEPPER_TRUE) {
		PEPPER_ERROR("Current input backend is already initialized\n");
		return 0;
	}

	input_db.init = PEPPER_TRUE;

	_headless_input_open_device("/dev/input/event1");

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return 1;
}

PEPPER_API int
headless_input_backend_fini(void *headless)
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);
	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return 0;
}
