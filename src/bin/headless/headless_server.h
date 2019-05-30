/*
* Copyright © 2018 Samsung Electronics co., Ltd. All Rights Reserved.
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

#ifndef HEADLESS_SERVER_H
#define HEADLESS_SERVER_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_bool_t pepper_output_led_init(pepper_compositor_t *compositor);
PEPPER_API void pepper_output_led_deinit(pepper_compositor_t *compositor);

PEPPER_API pepper_bool_t headless_shell_init(pepper_compositor_t *compositor);

/* APIs for headless_input */
PEPPER_API pepper_bool_t headless_input_init(pepper_compositor_t *compositor);
PEPPER_API void *headless_input_get(void);
PEPPER_API void headless_input_set_focus_view(void *input, pepper_view_t *view);
PEPPER_API void headless_input_set_top_view(void *input, pepper_view_t *view);

#ifdef __cplusplus
}
#endif

#endif /* HEADLESS_SERVER_H */

