/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
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

#ifndef PEPPER_KEYROUTER_H
#define PEPPER_KEYROUTER_H

#include <pepper.h>
#include <tizen-extension-server-protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PEPPER_KEYROUTER_MAX_KEYS 512

typedef struct pepper_keyrouter pepper_keyrouter_t;
typedef struct keyrouter_key_info keyrouter_key_info_t;

typedef enum keyrouter_grab_type {
	KEYROUTER_GRAB_TYPE_NONE,
	KEYROUTER_GRAB_TYPE_SHARED,
	KEYROUTER_GRAB_TYPE_TOP_POSITION,
	KEYROUTER_GRAB_TYPE_OR_EXCLUSIVE,
	KEYROUTER_GRAB_TYPE_EXCLUSIVE,
	KEYROUTER_GRAB_TYPE_REGISTERD
} keyrouter_grab_type_t;

typedef enum keyrouter_error {
	KEYROUTER_ERROR_NONE,
	KEYROUTER_ERROR_INVALID_SURFACE,
	KEYROUTER_ERROR_INVALID_KEY,
	KEYROUTER_ERROR_INVALID_MODE,
	KEYROUTER_ERROR_GRABBED_ALREADY,
	KEYROUTER_ERROR_NO_PERMISSION,
	KEYROUTER_ERROR_NO_SYSTEM_RESOURCES
} keyrouter_error_t;

struct keyrouter_key_info
{
	void *data;
	pepper_list_t link;
};

PEPPER_API pepper_keyrouter_t *pepper_keyrouter_create(void);
PEPPER_API void pepper_keyrouter_destroy(pepper_keyrouter_t *keyrouter);
PEPPER_API keyrouter_error_t pepper_keyrouter_grab_key(pepper_keyrouter_t *keyrouter, keyrouter_grab_type_t type, int keycode, void *data);
PEPPER_API void pepper_keyrouter_ungrab_key(pepper_keyrouter_t *keyrouter, keyrouter_grab_type_t type, int keycode, void *data);
PEPPER_API int pepper_keyrouter_key_process(pepper_keyrouter_t *keyrouter, int keycode, int pressed, pepper_list_t *delivery_list);

PEPPER_API void pepper_keyrouter_print_list(pepper_keyrouter_t *keyrouter);
#ifdef __cplusplus
}
#endif

#endif /* PEPPER_KEYROUTER_H */

