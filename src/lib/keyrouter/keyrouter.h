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

#ifndef KEYROUTER_H
#define KEYROUTER_H

#include <pepper.h>
#include <tizen-extension-server-protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYROUTER_MAX_KEYS 512

typedef struct keyrouter keyrouter_t;
typedef struct keyrouter_key_info keyrouter_key_info_t;

struct keyrouter_key_info {
	void *data;
	pepper_list_t link;
};

PEPPER_API keyrouter_t *keyrouter_create(void);
PEPPER_API void keyrouter_destroy(keyrouter_t *keyrouter);
PEPPER_API int keyrouter_grab_key(keyrouter_t *keyrouter, int type, int keycode, void *data);
PEPPER_API void keyrouter_ungrab_key(keyrouter_t *keyrouter, int type, int keycode, void *data);

#ifdef __cplusplus
}
#endif

#endif /* KEYROUTER_H */

