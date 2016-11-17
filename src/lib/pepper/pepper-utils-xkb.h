/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
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

#ifndef PEPPER_UTILS_XKB_H
#define PEPPER_UTILS_XKB_H

#include <unistd.h>
#include <sys/mman.h>
#include <pepper.h>
#include <pepper-utils.h>
#include <xkbcommon/xkbcommon.h>

struct pepper_xkb_info {
	struct xkb_state *state;
};

static inline struct pepper_xkb_info *
pepper_xkb_create(struct xkb_keymap *keymap)
{
	struct pepper_xkb_info *info = NULL;

	if (!keymap)
		return NULL;

	info = calloc(1, sizeof(struct pepper_xkb_info));
	info->state = xkb_state_new(keymap);

	return info;
}

static inline void
pepper_xkb_destroy(struct pepper_xkb_info *info)
{
	xkb_state_unref(info->state);
	free(info);
}

static inline void
pepper_xkb_get_modifier(struct pepper_xkb_info *info,
						uint32_t * depressed,
						uint32_t * latched,
						uint32_t * locked, uint32_t * group)
{
	if (depressed)
		*depressed = xkb_state_serialize_mods(info->state,
											  XKB_STATE_MODS_DEPRESSED);
	if (latched)
		*latched = xkb_state_serialize_mods(info->state,
											XKB_STATE_MODS_LATCHED);
	if (locked)
		*locked = xkb_state_serialize_mods(info->state,
										   XKB_STATE_MODS_LOCKED);
	if (group)
		*group = xkb_state_serialize_mods(info->state,
										  XKB_STATE_LAYOUT_EFFECTIVE);
}

static inline void
pepper_xkb_set_keyboard(struct pepper_xkb_info *info,
						pepper_keyboard_t * keyboard)
{
	struct xkb_keymap *keymap;
	char *keymap_str = NULL;
	int keymap_fd = -1;
	uint32_t keymap_len;
	char *keymap_map = NULL;

	uint32_t depressed = 0;
	uint32_t latched = 0;
	uint32_t locked = 0;
	uint32_t group = 0;

	if (!info || !keyboard)
		return;

	keymap = xkb_state_get_keymap(info->state);
	if (!keymap)
		return;

	keymap_str = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
	if (!keymap_str)
		goto err;

	keymap_len = strlen(keymap_str) + 1;
	keymap_fd = pepper_create_anonymous_file(keymap_len);
	if (keymap_fd < 0)
		goto err;

	keymap_map = mmap(NULL, keymap_len, PROT_READ | PROT_WRITE,
					  MAP_SHARED, keymap_fd, 0);
	if (!keymap_map)
		goto err;

	strncpy(keymap_map, keymap_str, keymap_len);
	pepper_keyboard_set_keymap_info(keyboard,
									WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
									keymap_fd, keymap_len);

	pepper_xkb_get_modifier(info, &depressed, &latched, &locked, &group);
	pepper_keyboard_set_modifiers(keyboard, depressed, latched, locked,
								  group);

  err:
	if (keymap_str)
		free(keymap_str);

	if (keymap_map)
		munmap(keymap_map, keymap_len);

	if (keymap_fd >= 0)
		close(keymap_fd);

	return;
}

static inline void
pepper_xkb_update_keyboard_modifier(struct pepper_xkb_info *info,
									pepper_keyboard_t * keyboard,
									pepper_input_event_t * event)
{
	enum xkb_key_direction direction;
	uint32_t depressed = 0;
	uint32_t latched = 0;
	uint32_t locked = 0;
	uint32_t group = 0;

	if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
		direction = XKB_KEY_DOWN;
	else
		direction = XKB_KEY_UP;

	xkb_state_update_key(info->state, event->key + 8, direction);

	pepper_xkb_get_modifier(info, &depressed, &latched, &locked, &group);
	pepper_keyboard_set_modifiers(keyboard, depressed, latched, locked,
								  group);
}

#endif
