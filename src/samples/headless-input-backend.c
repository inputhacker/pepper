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

#include <unistd.h>
#include <fcntl.h>

#include <pepper.h>
#include <pepper-input-backend.h>

#ifdef SIZE_BUFFER
# undef SIZE_BUFFER
#endif
#define SIZE_BUFFER 64

PEPPER_API int headless_input_backend_init(void *);
PEPPER_API int headless_input_backend_fini(void *);

static const char *_headless_input_device_get_property(void *device, const char *key);

typedef struct input_backend_data input_backend_data_t;
typedef struct device_data device_data_t;
typedef struct key_event key_event_t;

struct input_backend_data
{
	pepper_compositor_t *compositor;
	struct wl_display *display;
	struct wl_event_loop *event_loop;
	pepper_bool_t init;

	pepper_list_t device_list;
	pepper_list_t event_queue;
};

struct key_event
{
	int keycode;
	int state;
	pepper_input_device_t *device;

	pepper_list_t link;
};

struct device_data
{
	pepper_input_device_t *device;
	struct wl_event_source *event_source;

	pepper_list_t link;
};

input_backend_data_t input_info;

pepper_input_device_backend_t _headless_input_device_backend = {
	_headless_input_device_get_property
};

static const char *
_headless_input_device_get_property(void *device, const char *key)
{
	return NULL;
}

static unsigned long
_headless_input_get_current_time(void)
{
	struct timespec tp;

	if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
		return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000L);

	return 0;
}

static void
_headless_input_event_send(int keycode, int state, pepper_input_device_t *device)
{
	pepper_input_event_t event;

	event.time = _headless_input_get_current_time();
	event.key = keycode;

	if (state)
		event.state = PEPPER_KEY_STATE_PRESSED;
	else
		event.state = PEPPER_KEY_STATE_RELEASED;

	pepper_object_emit_event((pepper_object_t *)device,
	                         PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY, &event);
}

static void
_headless_input_event_queue(int keycode, int state, pepper_input_device_t * device)
{
	key_event_t *kinfo;
	kinfo = (key_event_t *)calloc(1, sizeof(key_event_t));
	PEPPER_CHECK(kinfo, return, "Failed to alloc for key event\n");

	pepper_list_init(&kinfo->link);
	kinfo->keycode = keycode;
	kinfo->state = state;
	kinfo->device = device;

	pepper_list_insert(&input_info.event_queue, &kinfo->link);
}

static void
_headless_input_event_flush(void)
{
	key_event_t *kinfo, *ktmp;

	pepper_list_for_each_safe(kinfo, ktmp, &input_info.event_queue, link)
	{
		_headless_input_event_send(kinfo->keycode, kinfo->state, kinfo->device);
		pepper_list_remove(&kinfo->link);
		free(kinfo);
	}
}

static void
_headless_input_event_process(struct input_event *ev, pepper_input_device_t *device)
{
	switch (ev->type)
	{
		case EV_KEY:
			_headless_input_event_queue(ev->code, ev->value, device);
			break;

		case EV_SYN:
			_headless_input_event_flush();
			break;

		default:
			break;
	}
}

static void
_headless_input_close_device(pepper_input_device_t *device)
{
	device_data_t *dinfo, *dtmp;

	pepper_list_for_each_safe(dinfo, dtmp, &input_info.device_list, link)
	{
		if (dinfo->device == device)
		{
			wl_event_source_remove(dinfo->event_source);
			pepper_input_device_destroy(dinfo->device);
			pepper_list_remove(&dinfo->link);
			free(dinfo);
		}
	}
}

static int
_headless_input_backend_event_loop(int fd, uint32_t mask, void *data)
{
	struct input_event iev[SIZE_BUFFER];
	int i, len;

	if (mask & WL_EVENT_READABLE)
	{
		memset(iev, 0, sizeof(struct input_event)*SIZE_BUFFER);

		if ((len = read(fd,  &iev, sizeof(iev))) != -1)
		{
			for (i = 0; i < (int)(len / sizeof(iev[0])); i++)
			{
				_headless_input_event_process(&iev[i], (pepper_input_device_t *)data);
			}
		}
	}

	if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR))
	{
		_headless_input_close_device((pepper_input_device_t *)data);
	}

	return 1;
}

static void
_headless_input_open_device(const char *path)
{
	int fd;
	device_data_t *device_info;

	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		PEPPER_ERROR("Failed to open %s\n", path);
		return;
	}

	device_info = (device_data_t *)calloc(1, sizeof(device_data_t));
	PEPPER_CHECK(device_info, return, "Failed to allocate memory for device\n");

	pepper_list_init(&device_info->link);
	device_info->device = pepper_input_device_create(input_info.compositor, WL_SEAT_CAPABILITY_KEYBOARD,
	                                                 &_headless_input_device_backend, NULL);
	PEPPER_CHECK(device_info->device, goto failed, "Failed to create pepper input device\n");

	device_info->event_source = wl_event_loop_add_fd(input_info.event_loop, fd,
								WL_EVENT_READABLE | WL_EVENT_HANGUP | WL_EVENT_ERROR,
	                                                 _headless_input_backend_event_loop, device_info->device);
	PEPPER_CHECK(device_info->event_source, goto failed, "Failed to create pepper input device\n");

	pepper_list_insert(&input_info.device_list, &device_info->link);

	return;

failed:

	if (device_info->device)
	{
		pepper_input_device_destroy(device_info->device);
		device_info->device = NULL;
	}

	if (device_info)
	{
		free(device_info);
		device_info = NULL;
	}

	if (fd >= 0)
		close(fd);
}

PEPPER_API int
headless_input_backend_init(void *headless)
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	input_info.compositor = (pepper_compositor_t *)headless;
	PEPPER_CHECK(input_info.compositor, return 0, "Invalid compositor\n");

	input_info.display = pepper_compositor_get_display(input_info.compositor);
	PEPPER_CHECK(input_info.display, return 0, "Invalid wl_display\n");

	input_info.event_loop = wl_display_get_event_loop(input_info.display);
	PEPPER_CHECK(input_info.event_loop, return 0, "No event loop exist.\n");

	if (input_info.init == PEPPER_TRUE)
	{
		PEPPER_ERROR("Current input backend is already initialized\n");
		return 0;
	}

	pepper_list_init(&input_info.event_queue);
	pepper_list_init(&input_info.device_list);

	input_info.init = PEPPER_TRUE;

	_headless_input_open_device("/dev/input/event0");
	_headless_input_open_device("/dev/input/event1");

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return 1;
}

PEPPER_API int
headless_input_backend_fini(void *headless)
{
	device_data_t *dinfo, *dtmp;

	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	/* Flush queued event */
	if (!pepper_list_empty(&input_info.event_queue))
	{
		_headless_input_event_flush();
	}

	if (!pepper_list_empty(&input_info.device_list))
	{
		pepper_list_for_each_safe(dinfo, dtmp, &input_info.device_list, link)
		{
			wl_event_source_remove(dinfo->event_source);
			pepper_input_device_destroy(dinfo->device);
			pepper_list_remove(&dinfo->link);
			free(dinfo);
		}
	}

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return 0;
}
