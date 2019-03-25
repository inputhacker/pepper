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

#ifndef PEPPER_INOTIFY_INTERNAL_H
#define PEPPER_INOTIFY_INTERNAL_H

#include <pepper.h>
#include <pepper-inotify.h>

#define MAX_PATH_LEN 64

struct pepper_inotify_event
{
	pepper_inotify_t *inotify;

	char *name;
	pepper_bool_t is_dir;
};

struct pepper_inotify_watch
{
	pepper_inotify_t *inotify;
	struct wl_event_source *event_source;
	int wd;
	char path[MAX_PATH_LEN];

	pepper_list_t link;
};

struct pepper_inotify
{
	pepper_compositor_t *compositor;
	struct wl_display *display;
	struct wl_event_loop *event_loop;

	struct {
		pepper_inotify_event_cb_t cb;
		void *data;
	} callback;

	int fd;

	pepper_list_t watched_list;
};

#endif /* PEPPER_INOTIFY_INTERNAL_H */
