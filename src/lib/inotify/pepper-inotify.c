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
#include <sys/inotify.h>

#include <pepper-inotify-internal.h>

static void
_inotify_event_post(pepper_inotify_watch_t *watch, pepper_inotify_event_type_t type, const char *name, pepper_bool_t is_dir)
{
	pepper_inotify_event_t ev;

	PEPPER_CHECK(watch->inotify->callback.cb, return, "Current inotify has no callback");

	ev.inotify = watch->inotify;

	ev.name = (char *)name;
	ev.is_dir = is_dir;
	ev.path = watch->path;

	watch->inotify->callback.cb(type, &ev, watch->inotify->callback.data);
}

static void
_inotify_event_process(struct inotify_event *ev, pepper_inotify_watch_t *watch)
{
	if (!ev->len) return;

	if (ev->mask & IN_CREATE) {
		_inotify_event_post(watch, PEPPER_INOTIFY_EVENT_TYPE_CREATE,
					ev->name, (ev->mask & IN_ISDIR) ? PEPPER_TRUE : PEPPER_FALSE);
	}
	else if (ev->mask & IN_DELETE) {
		_inotify_event_post(watch, PEPPER_INOTIFY_EVENT_TYPE_REMOVE,
					ev->name, (ev->mask & IN_ISDIR) ? PEPPER_TRUE : PEPPER_FALSE);
	}
	else if (ev->mask & IN_MODIFY) {
		_inotify_event_post(watch, PEPPER_INOTIFY_EVENT_TYPE_MODIFY,
					ev->name, (ev->mask & IN_ISDIR) ? PEPPER_TRUE : PEPPER_FALSE);
	}
}

static int
_inotify_fd_read(int fd, uint32_t mask, void *data)
{
	uint32_t i;
	int nread;
	char buf[128];
	struct inotify_event ev[32];
	pepper_inotify_watch_t *watch_data = data;

	PEPPER_CHECK(!(mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)),
			return 0,
			"[%s] With the given fd, there is an error or it's been hung-up.\n",
			__FUNCTION__);

	if (!(mask & WL_EVENT_READABLE))
		return 0;

	nread = read(fd, &ev, sizeof(ev));
	PEPPER_CHECK(nread >= 0, return 0,
			"Failed on reading given fd. (error : %s, fd:%d)\n",
			strerror_r(errno, buf, 128), fd);

	for (i = 0 ; i < (nread / sizeof(ev[0])); i++) {
		if (ev[i].wd == watch_data->wd)
			_inotify_event_process(&ev[i], watch_data);
	}

	return 0;
}

PEPPER_API char *
pepper_inotify_event_name_get(pepper_inotify_event_t *ev)
{
	PEPPER_CHECK(ev, return NULL,
			"Invalid inotify event\n");

	return ev->name;
}

PEPPER_API char *
pepper_inotify_event_path_get(pepper_inotify_event_t *ev)
{
	PEPPER_CHECK(ev, return NULL,
			"Invalid inotify event\n");

	return ev->path;
}

PEPPER_API pepper_bool_t
pepper_inotify_event_is_directory(pepper_inotify_event_t *ev)
{
	PEPPER_CHECK(ev, return PEPPER_FALSE,
			"Invalid inotify event\n");

	return ev->is_dir;
}

PEPPER_API pepper_bool_t
pepper_inotify_add(pepper_inotify_t *inotify, const char *path)
{
	pepper_inotify_watch_t *watch_data;
	char buf[128];

	PEPPER_CHECK(inotify, return PEPPER_FALSE, "Invalid pepper_inotify_t object\n");
	PEPPER_CHECK(path, return PEPPER_FALSE, "Invalid path\n");

	watch_data = (pepper_inotify_watch_t *)calloc(1, sizeof(pepper_inotify_watch_t));
	PEPPER_CHECK(watch_data, return PEPPER_FALSE,
		"Failed to allocate pepper_inotify_watch_t\n");

	watch_data->inotify = inotify;

	watch_data->wd = inotify_add_watch(inotify->fd, path,
			IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF);
	PEPPER_CHECK(watch_data->wd >= 0, goto failed,
			"Failed to add watch for %s\n", path, strerror_r(errno, buf, 128));

	strncpy(watch_data->path, path, MAX_PATH_LEN - 1);

	watch_data->event_source = wl_event_loop_add_fd(inotify->event_loop, inotify->fd,
			WL_EVENT_READABLE, _inotify_fd_read, watch_data);
	PEPPER_CHECK(watch_data->event_source, goto failed,
			"Failed to add fd as an event source.\n");

	pepper_list_insert(&inotify->watched_list, &watch_data->link);

	return PEPPER_TRUE;

failed:
	if (watch_data->event_source)
		wl_event_source_remove(watch_data->event_source);
	if (watch_data->wd)
		inotify_rm_watch(inotify->fd, watch_data->wd);

	return PEPPER_FALSE;
}

PEPPER_API void
pepper_inotify_del(pepper_inotify_t *inotify, const char *path)
{
	pepper_inotify_watch_t *watch_data, *watch_data_tmp;

	PEPPER_CHECK(inotify, return, "Invalid pepper_inotify_t object\n");
	PEPPER_CHECK(path, return, "Invalid path\n");

	if (!pepper_list_empty(&inotify->watched_list))
	{
		pepper_list_for_each_safe(watch_data, watch_data_tmp, &inotify->watched_list, link)
		{
			if (!strncmp(watch_data->path, path, MAX_PATH_LEN - 1))
			{
				if (watch_data->wd)
					inotify_rm_watch(inotify->fd, watch_data->wd);
				if (watch_data->event_source)
					wl_event_source_remove(watch_data->event_source);
				pepper_list_remove(&watch_data->link);
				free(watch_data);
				break;
			}
		}
	}
}

PEPPER_API pepper_inotify_t *
pepper_inotify_create(pepper_compositor_t *compositor, pepper_inotify_event_cb_t cb, void *data)
{
	pepper_inotify_t *inotify;

	PEPPER_CHECK(compositor, return NULL, "Invalid compositor object\n");

	inotify = (pepper_inotify_t *)calloc(1, sizeof(pepper_inotify_t));
	PEPPER_CHECK(inotify, return NULL, "Failed to allocate pepper_inotify_t\n");

	inotify->display = pepper_compositor_get_display(compositor);
	PEPPER_CHECK(inotify->display, goto failed, "Invaild wl_display\n");

	inotify->event_loop = wl_display_get_event_loop(inotify->display);
	PEPPER_CHECK(inotify->event_loop, goto failed, "Invaild wl_event_loop\n");

	inotify->fd = inotify_init();
	PEPPER_CHECK(inotify->fd >= 0, goto failed, "Failed to init inotify. fd: %d\n", inotify->fd);

	inotify->callback.cb = cb;
	inotify->callback.data = data;

	pepper_list_init(&inotify->watched_list);

	return inotify;

failed:
	if (inotify) {
		if (inotify->fd)
			close(inotify->fd);
		free(inotify);
	}
	return NULL;
}

PEPPER_API void
pepper_inotify_destroy(pepper_inotify_t *inotify)
{
	pepper_inotify_watch_t *watch_data, *watch_data_tmp;

	PEPPER_CHECK(inotify, return, "Invalid pepper_inotify_t object\n");

	if (!pepper_list_empty(&inotify->watched_list))
	{
		pepper_list_for_each_safe(watch_data, watch_data_tmp, &inotify->watched_list, link)
		{
			if (watch_data->wd)
				inotify_rm_watch(inotify->fd, watch_data->wd);
			if (watch_data->event_source)
				wl_event_source_remove(watch_data->event_source);

			pepper_list_remove(&watch_data->link);
			free(watch_data);
		}

		pepper_list_remove(&inotify->watched_list);
	}

	if (inotify->fd)
		close(inotify->fd);

	free(inotify);
	inotify = NULL;
}

