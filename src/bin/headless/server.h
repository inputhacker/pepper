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

#ifndef _PEPPER_HEADLESS_SERVER_H
#define _PEPPER_HEADLESS_SERVER_H

#include <pepper.h>

#ifdef PATH_MAX
#undef PATH_MAX
#define PATH_MAX 1024
#endif

#ifdef SYM_MAX
#undef SYM_MAX
#define SYM_MAX 1024
#endif

const char *module_basedir = "/usr/lib/pepper/modules/";

typedef struct headless_io_backend_func headless_io_backend_func_t;

typedef struct headless_io_backend_func
{
	int (*backend_init)(void *headless);
	int (*backend_fini)(void *headless);
};

#endif /* _PEPPER_HEADLESS_SERVER_H */
