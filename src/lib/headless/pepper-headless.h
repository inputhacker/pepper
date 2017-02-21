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

#ifndef PEPPER_HEADLESS_H
#define PEPPER_HEADLESS_H

#include <pepper.h>
#include <headless-internal.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pepper_headless_resource
{
	struct wl_resource *resource;
	pepper_list_t link;
};

struct pepper_headless_buffer
{
	struct wl_resource *buffer;
	unsigned int type;
	void *data;
	pepper_list_t link;
};

struct pepper_headless {
	struct wl_global *global;
	struct wl_display *display;
	pepper_compositor_t *compositor;

	pepper_headless_output_backend_t *output_backend;

	pepper_list_t resources;
	pepper_list_t buffers;
};

PEPPER_API struct wl_display *
pepper_headless_get_display(pepper_headless_t *headless);

PEPPER_API pepper_compositor_t *
pepper_headless_get_compositor(pepper_headless_t *headless);

PEPPER_API pepper_headless_output_backend_t *
pepper_headless_get_output_backend(pepper_headless_t *headless);

PEPPER_API pepper_headless_t *
pepper_headless_create(pepper_compositor_t *compositor);

PEPPER_API void
pepper_headless_destroy(pepper_headless_t *headless);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_HEADLESS_H */
