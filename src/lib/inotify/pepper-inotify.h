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

#ifndef PEPPER_INOTIFY_H
#define PEPPER_INOTIFY_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_inotify pepper_inotify_t;
typedef struct pepper_inotify_watch pepper_inotify_watch_t;
typedef struct pepper_inotify_event pepper_inotify_event_t;

typedef void (*pepper_inotify_event_cb_t)(uint32_t type, pepper_inotify_event_t *ev, void *data);

typedef enum pepper_inotify_event_type
{
	PEPPER_INOTIFY_EVENT_TYPE_NONE,
	PEPPER_INOTIFY_EVENT_TYPE_CREATE,
	PEPPER_INOTIFY_EVENT_TYPE_REMOVE,
	PEPPER_INOTIFY_EVENT_TYPE_MODIFY
} pepper_inotify_event_type_t;

PEPPER_API pepper_bool_t
pepper_inotify_add(pepper_inotify_t *inotify, const char *path);

PEPPER_API void
pepper_inotify_del(pepper_inotify_t *inotify, const char *path);

PEPPER_API pepper_inotify_t *
pepper_inotify_create(pepper_compositor_t *compositor, pepper_inotify_event_cb_t cb, void *data);

PEPPER_API void
pepper_inotify_destroy(pepper_inotify_t *inotify);

PEPPER_API pepper_inotify_event_type_t
pepper_inotify_event_type_get(pepper_inotify_event_t *ev);

PEPPER_API char *
pepper_inotify_event_name_get(pepper_inotify_event_t *ev);

PEPPER_API char *
pepper_inotify_event_path_get(pepper_inotify_event_t *ev);

PEPPER_API pepper_bool_t
pepper_inotify_event_is_directory(pepper_inotify_event_t *ev);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_INOTIFY_H */
