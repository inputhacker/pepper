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

#ifndef DEVICEMGR_H
#define DEVICEMGR_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct devicemgr devicemgr_t;
typedef struct devicemgr_device devicemgr_device_t;

PEPPER_API devicemgr_t *devicemgr_create(pepper_compositor_t *compositor, pepper_seat_t *seat);
PEPPER_API void devicemgr_destroy(devicemgr_t *devicemgr);
PEPPER_API int devicemgr_input_generator_init(devicemgr_t *devicemgr, unsigned int clas, const char *name);
PEPPER_API int devicemgr_input_generator_deinit(devicemgr_t *devicemgr);
PEPPER_API int devicemgr_input_generator_generate_key(devicemgr_t *devicemgr, int keycode, pepper_bool_t pressed);

PEPPER_API void devicemgr_input_generator_keyboard_set(devicemgr_t *devicemgr, pepper_keyboard_t *keyboard);
PEPPER_API void devicemgr_input_generator_keyboard_unset(devicemgr_t *devicemgr, pepper_keyboard_t *keyboard);

#ifdef __cplusplus
}
#endif

#endif /* DEVICEMGR_H */

