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

#ifndef PEPPER_HEADLESS_OUTPUT_H
#define PEPPER_HEADLESS_OUTPUT_H

#include <headless-internal.h>
#include <pepper-headless.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_headless_output_backend pepper_headless_output_backend_t;

struct pepper_headless_output_backend
{
	pepper_headless_output_backend_t* (*backend_init)(pepper_headless_t *);
	void (*backend_fini)(pepper_headless_output_backend_t *);

	pepper_bool_t (*display_buffer)(void *data);
};

PEPPER_API pepper_headless_output_backend_t *backend_alloc(void);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_HEADLESS_OUTPUT_H */