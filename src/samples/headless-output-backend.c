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

//pepper_output_device_create

#include <pepper.h>
#include <pepper-headless.h>
#include <pepper-headless-output.h>

//Prototype
PEPPER_API pepper_bool_t headless_output_display_buffer(void *);
PEPPER_API pepper_headless_output_backend_t* headless_output_backend_init(pepper_headless_t *);
PEPPER_API void headless_output_backend_fini(pepper_headless_output_backend_t *);

//Implementation
PEPPER_API pepper_bool_t
headless_output_display_buffer(void *data)
{
	pepper_bool_t ret = PEPPER_FALSE;

	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	//TODO : display buffer data to the output(s)
	PEPPER_TRACE("%s : display buffer data to output(s)...\n", __FUNCTION__);

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return ret;
}

PEPPER_API pepper_headless_output_backend_t *
headless_output_backend_init(pepper_headless_t *headless)
{
	pepper_headless_output_backend_t *backend = NULL;

	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	backend = (pepper_headless_output_backend_t *)calloc(1, sizeof(pepper_headless_output_backend_t));
	PEPPER_CHECK(backend, goto error, "Failed to allocate memory for headless output backend...\n");

//	backend->backend_init = headless_output_backend_init;
	backend->backend_fini = headless_output_backend_fini;
	backend->display_buffer = headless_output_display_buffer;

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return backend;

error:
	if (backend)
	{
		free(backend);
		backend = NULL;
	}

	return NULL;
}

PEPPER_API void
headless_output_backend_fini(pepper_headless_output_backend_t* backend)
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);
	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return;
}
