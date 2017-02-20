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

#ifndef PEPPER_HEADLESS_INTERNAL_H
#define PEPPER_HEADLESS_INTERNAL_H

#include "pepper-headless.h"

struct pepper_headless_resource
{
	struct wl_resource *resource;
	pepper_list_t link;
};

struct pepper_headless_buffer
{
	struct wl_resource *buffer;
	enum tizen_headless_buffer_type type;
	void *data;
	pepper_list_t link;
};

struct pepper_headless {
	struct wl_global *global;
	struct wl_display *display;
	pepper_compositor_t *compositor;

	pepper_list_t resources;
	pepper_list_t buffers;
};

#endif /* PEPPER_HEADLESS_INTERNAL_H */

