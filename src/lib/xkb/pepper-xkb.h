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

#ifndef PEPPER_XKB_H
#define PEPPER_XKB_H

#include <pepper.h>
#include <xkbcommon/xkbcommon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xkb_rule_names pepper_xkb_rule_names;
typedef pepper_xkb_rule_names pepper_xkb_rule_names_t;
typedef struct pepper_xkb_info pepper_xkb_info_t;
typedef struct pepper_xkb pepper_xkb_t;

PEPPER_API struct xkb_keymap *pepper_xkb_get_keymap(pepper_xkb_t *xkb);
PEPPER_API struct xkb_context *pepper_xkb_get_context(pepper_xkb_t *xkb);
PEPPER_API struct xkb_state *pepper_xkb_get_state(pepper_xkb_t *xkb);

PEPPER_API void
pepper_xkb_keyboard_set_keymap(pepper_xkb_t *xkb,
										pepper_keyboard_t *keyboard,
										pepper_xkb_rule_names_t *names);

PEPPER_API pepper_xkb_t *
pepper_xkb_create(void);

PEPPER_API void
pepper_xkb_destroy(pepper_xkb_t *xkb);

PEPPER_API int
pepper_xkb_info_keyname_to_keycode(pepper_xkb_info_t *xkb_info, const char *keyname);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_XKB_H */
