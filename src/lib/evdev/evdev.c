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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/input.h>

#include <evdev-internal.h>
#include <pepper-input-backend.h>

#ifdef EVENT_MAX
#undef EVENT_MAX
#endif
#define EVENT_MAX 32

#ifndef LONG_BITS
#define LONG_BITS (sizeof(long) * 8)
#endif

#ifndef NLONGS
#define NLONGS(x) (((x) + LONG_BITS - 1) / LONG_BITS)
#endif

static void
_evdev_keyboard_event_post(pepper_input_device_t *device, uint32_t keycode, int state, uint32_t time)
{
	pepper_input_event_t event;

	event.time = time;
	event.key = keycode;
	event.state = state ? PEPPER_KEY_STATE_PRESSED : PEPPER_KEY_STATE_RELEASED;

	pepper_object_emit_event((pepper_object_t *)device,
								PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY, &event);
}

static void
_evdev_keyboard_event_flush(pepper_evdev_t *evdev)
{
	evdev_key_event_t *event = NULL;
	evdev_key_event_t *tmp = NULL;

	pepper_list_for_each_safe(event, tmp, &evdev->key_event_queue, link)
	{
		_evdev_keyboard_event_post(event->device, event->keycode, event->state, event->time);
		pepper_list_remove(&event->link);
		free(event);
	}
}

static void
_evdev_keyboard_event_queue(uint32_t keycode, int state, uint32_t time, evdev_device_info_t *device_info)
{
	evdev_key_event_t *event = NULL;
	pepper_evdev_t *evdev = device_info->evdev;

	event = (evdev_key_event_t *)calloc(1, sizeof(evdev_key_event_t));
	PEPPER_CHECK(event, return, "[%s] Failed to allocate memory for key event.\n", __FUNCTION__);

	event->keycode = keycode;
	event->state = state;
	event->time = time;
	event->device = device_info->device;

	pepper_list_insert(&evdev->key_event_queue, &event->link);
}

static void
_evdev_keyboard_event_process(struct input_event *ev, evdev_device_info_t *device_info)
{
	 uint32_t timestamp;

	/* FIXME : need to think about using current time vs. time within event from kernel */
	timestamp = ev->time.tv_sec * 1000 + ev->time.tv_usec / 1000;

	switch (ev->type)
	{
		case EV_KEY:
			_evdev_keyboard_event_queue((uint32_t)ev->code, ev->value, timestamp, device_info);
			break;

		case EV_SYN:
			_evdev_keyboard_event_flush(device_info->evdev);
			break;

		default:
			break;
	}
}

static int
_evdev_keyboard_event_fd_read(int fd, uint32_t mask, void *data)
{
	uint32_t i;
	int nread;
	char buf[128];
	struct input_event ev[EVENT_MAX];
	evdev_device_info_t *device_info = (evdev_device_info_t *)data;

	PEPPER_CHECK(!(mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)),
					return 0,
					"[%s] With the given fd, there is an error or it's been hung-up.\n",
					__FUNCTION__);

	if (!(mask & WL_EVENT_READABLE))
		return 0;

	nread = read(fd, &ev, sizeof(ev));
	PEPPER_CHECK(nread>=0, return 0, "[%s] Failed on reading given fd. (error : %s, fd:%d)\n",
					__FUNCTION__, strerror_r(errno, buf, 128), fd);

	for (i = 0 ; i < (nread / sizeof(ev[0])); i++)
	{
		_evdev_keyboard_event_process(&ev[i], device_info);
	}

	return 0;
}

static int
_evdev_is_key_device(int fd, const char *path)
{
	int rc;
	unsigned long key_bits[NLONGS(KEY_CNT)];

	if (fd < 0 || !path)
		return 0;

	/* Check if the given device is a key device */
	rc = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
	if (rc < 0)
		return 0;

	return 1;
}

static int
_evdev_is_mouse_device(int fd, const char *path)
{
	int rc;
	char device_name[256];
	unsigned long rel_bits[NLONGS(REL_CNT)];

	if (fd < 0 || !path)
		return 0;

	/* Check if the given device is a mouse device */
	rc = ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits);
	if (rc < 0)
		return 0;

	rc = ioctl(fd, EVIOCGNAME(sizeof(device_name)-1), device_name);
	if (rc < 0 || !strcasestr(device_name, "mouse"))
		return 0;

	return 1;
}

static int
_evdev_keyboard_device_open(pepper_evdev_t *evdev, const char *path)
{
	int fd;
	char device_path[32];
	uint32_t event_mask;
	evdev_device_info_t *device_info = NULL;
	pepper_input_device_t *device = NULL;

	PEPPER_CHECK(path, return 0, "[%s] Given path is NULL.\n", __FUNCTION__);

	snprintf(device_path, sizeof(device_path), "/dev/input/%s", path);

	fd = open(device_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	PEPPER_CHECK(fd >= 0, return 0, "[%s] Failed to open given path of device.\n", __FUNCTION__);

	/* Skip mouse device as it usually has a key capability */
	if (!_evdev_is_key_device(fd, path) || _evdev_is_mouse_device(fd, path))
		goto error;

	device = pepper_input_device_create(evdev->compositor, WL_SEAT_CAPABILITY_KEYBOARD, NULL, NULL);
	PEPPER_CHECK(device, goto error, "[%s] Failed to create pepper input device.\n", __FUNCTION__);

	device_info = (evdev_device_info_t *)calloc(1, sizeof(evdev_device_info_t));
	PEPPER_CHECK(device_info, goto error, "[%s] Failed to allocate memory for device info...\n", __FUNCTION__);

	event_mask = WL_EVENT_READABLE;
	device_info->evdev = evdev;
	device_info->device = device;
	device_info->event_source = wl_event_loop_add_fd(evdev->event_loop,
			fd, event_mask, _evdev_keyboard_event_fd_read, device_info);
	PEPPER_CHECK(device_info->event_source, goto error, "[%s] Failed to add fd as an event source...\n", __FUNCTION__);

	pepper_list_insert(&evdev->device_list, &device_info->link);

	return 1;

error:
	if (device)
	{
		pepper_input_device_destroy(device);
		device = NULL;
	}

	if (device_info)
	{
		if (device_info->event_source)
			wl_event_source_remove(device_info->event_source);

		free(device_info);
		device_info = NULL;
	}

	if (fd >=0)
		close(fd);

	return 0;
}

PEPPER_API uint32_t
pepper_evdev_device_probe(pepper_evdev_t *evdev, uint32_t caps)
{
	uint32_t probed = 0;

	DIR *dir_info = NULL;
	struct dirent *dir_entry = NULL, dir_prev;
	int ret = 0;

	/* Probe event device nodes under /dev/input */
	dir_info = opendir("/dev/input/");

	if (dir_info)
	{
		ret = readdir_r(dir_info, &dir_prev, &dir_entry);
		while ((ret == 0) && (dir_entry != NULL))
		{
			if (!strncmp(dir_entry->d_name, "event", 5))
			{
				if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
					probed += _evdev_keyboard_device_open(evdev, dir_entry->d_name);
			}
			dir_prev = *dir_entry;
			ret = readdir_r(dir_info, &dir_prev, &dir_entry);
		}

		closedir(dir_info);
		dir_info = NULL;
	}

	return probed;
}

PEPPER_API pepper_evdev_t *
pepper_evdev_create(pepper_compositor_t *compositor)
{
	pepper_evdev_t *evdev = NULL;

	evdev = (pepper_evdev_t *)calloc(1, sizeof(pepper_evdev_t));
	PEPPER_CHECK(evdev, return NULL, "[%s] Failed to allocate memory for pepper evdev...\n", __FUNCTION__);

	evdev->compositor = compositor;
	evdev->display = pepper_compositor_get_display(compositor);
	evdev->event_loop = wl_display_get_event_loop(evdev->display);

	pepper_list_init(&evdev->device_list);
	pepper_list_init(&evdev->key_event_queue);

	return evdev;
}

PEPPER_API void
pepper_evdev_destroy(pepper_evdev_t *evdev)
{
	evdev_device_info_t *device_info = NULL;
	evdev_device_info_t *tmp = NULL;

	if (!evdev)
		return;

	/* clean-up/destroy key event queue */
	if (!pepper_list_empty(&evdev->key_event_queue))
	{
		_evdev_keyboard_event_flush(evdev);
		pepper_list_remove(&evdev->key_event_queue);
	}

	/* clean-up/destory device list */
	if (!pepper_list_empty(&evdev->device_list))
	{
		pepper_list_for_each_safe(device_info, tmp, &evdev->device_list, link)
		{
			if (device_info->device)
				pepper_input_device_destroy(device_info->device);
			if (device_info->event_source)
				wl_event_source_remove(device_info->event_source);

			pepper_list_remove(&device_info->link);
			free(device_info);
		}

		pepper_list_remove(&evdev->device_list);
	}

	free(evdev);
}

