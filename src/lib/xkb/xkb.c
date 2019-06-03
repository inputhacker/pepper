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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>

#include <pepper-utils.h>
#include <xkb-internal.h>

typedef struct _keycode_map{
    xkb_keysym_t keysym;
    xkb_keycode_t keycode;
} keycode_map;

static void
find_keycode(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
	keycode_map *found_keycodes = (keycode_map *)data;
	xkb_keysym_t keysym = found_keycodes->keysym;
	int nsyms = 0;
	const xkb_keysym_t *syms_out = NULL;

	if (found_keycodes->keycode) return;

	nsyms = xkb_keymap_key_get_syms_by_level(keymap, key, 0, 0, &syms_out);
	if (nsyms && syms_out) {
		if (*syms_out == keysym) {
			found_keycodes->keycode = key;
		}
	}
}

static void
_pepper_xkb_keycode_from_keysym(struct xkb_keymap *keymap, xkb_keysym_t keysym, xkb_keycode_t *keycode)
{
	keycode_map found_keycodes = {0,};
	found_keycodes.keysym = keysym;
	xkb_keymap_key_for_each(keymap, find_keycode, &found_keycodes);

	*keycode = found_keycodes.keycode;
}

PEPPER_API int
pepper_xkb_info_keyname_to_keycode(pepper_xkb_info_t *xkb_info, const char *keyname)
{
	xkb_keysym_t keysym = 0x0;
	xkb_keycode_t keycode = 0;

	PEPPER_CHECK(xkb_info, return -1, "pepper xkb is not set\n");

	if (!strncmp(keyname, "Keycode-", sizeof("Keycode-")-1)) {
		keycode = atoi(keyname+8);
	} else {
		keysym = xkb_keysym_from_name(keyname, XKB_KEYSYM_NO_FLAGS);
		_pepper_xkb_keycode_from_keysym(xkb_info->keymap, keysym, &keycode);
	}

	return keycode;
}

static void
pepper_xkb_info_destroy(pepper_xkb_info_t *info)
{
	if (!info)
		return;

	xkb_keymap_unref(info->keymap);
	xkb_state_unref(info->state);
	xkb_context_unref(info->context);

	free(info);
}

static struct xkb_keymap *
pepper_xkb_keymap_create(struct xkb_context* context,
								pepper_xkb_rule_names_t *names)
{
	struct xkb_keymap* keymap;

	if (!context)
		return NULL;

	keymap = xkb_map_new_from_names(context, names, XKB_KEYMAP_COMPILE_NO_FLAGS);

	return keymap;
}

static void
pepper_xkb_get_modifier(pepper_xkb_info_t *info,
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

static void
pepper_xkb_update_keyboard_modifier(pepper_xkb_info_t *info,
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

static void
_pepper_xkb_handle_keyboard_key(pepper_event_listener_t *listener,
							pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_xkb_info_t *xkb_info = NULL;

	PEPPER_CHECK(id == PEPPER_EVENT_KEYBOARD_KEY, return, "%d event will not be handled.\n", id);
	PEPPER_CHECK(info, return, "Invalid event\n");
	PEPPER_CHECK(data, return, "Invalid data.\n");

	pepper_input_event_t *event = (pepper_input_event_t *)info;
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)data;

	xkb_info = pepper_keyboard_get_xkb_info(keyboard);
	if (xkb_info)
		pepper_xkb_update_keyboard_modifier(xkb_info, keyboard, event);
}

static void
_pepper_xkb_handle_keymap_update(pepper_event_listener_t *listener,
							pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_xkb_info_t *cur_xkb_info = NULL;
	pepper_xkb_info_t *pending_xkb_info = NULL;
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)info;

	PEPPER_CHECK(id == PEPPER_EVENT_KEYBOARD_KEYMAP_UPDATE, return, "%d event will not be handled.\n", id);
	PEPPER_CHECK(info, return, "Invalid event\n");
	PEPPER_CHECK(data, return, "Invalid data.\n");

	pending_xkb_info = pepper_keyboard_get_pending_xkb_info(keyboard);

	if (!pending_xkb_info)
	{
		PEPPER_ERROR("Pending xkb_info of a pepper keyboard is invalid !\n");
		return;
	}

	/* destroy existing xkb information */
	cur_xkb_info = pepper_keyboard_get_xkb_info(keyboard);

	if (cur_xkb_info)
		pepper_xkb_info_destroy(cur_xkb_info);

	/* update xkb information with pending xkb information */
	pepper_keyboard_set_xkb_info(keyboard, pending_xkb_info);
	pepper_keyboard_set_pending_xkb_info(keyboard, NULL);
}

static pepper_xkb_info_t *
pepper_xkb_create_with_names(pepper_xkb_rule_names_t *names)
{
	 pepper_xkb_info_t *info = NULL;

	 info = calloc(1, sizeof(struct pepper_xkb_info));

	 if (!info)
	 	return NULL;

	info->context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!info->context)
		goto err;

	info->keymap = pepper_xkb_keymap_create(info->context, names);
	if (!info->keymap)
		goto err;

	info->state = xkb_state_new(info->keymap);
	if (!info->state)
		goto err;

	return info;

err:
	xkb_keymap_unref(info->keymap);
	xkb_state_unref(info->state);
	xkb_context_unref(info->context);
	free(info);

	return NULL;
}

static pepper_xkb_info_t *
pepper_xkb_info_create_with_names(pepper_xkb_rule_names_t *names)
{
	pepper_xkb_rule_names_t *xkb_names = NULL;
	pepper_xkb_rule_names_t default_names = { 0, };

	if (names)
		xkb_names = names;
	else
	{
		/* set default names */
		default_names.rules = "evdev";
		default_names.model = "pc105";
		default_names.layout = "us";
		default_names.variant = NULL;
		default_names.options = NULL;

		xkb_names = &default_names;
	}

	return pepper_xkb_create_with_names(xkb_names);
}

static void
pepper_xkb_set_keyboard(pepper_xkb_t *xkb, pepper_xkb_info_t *info,
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

	if (!xkb || !info || !keyboard)
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

	xkb->listener_key = pepper_object_add_event_listener((pepper_object_t *)keyboard,
								PEPPER_EVENT_KEYBOARD_KEY, 0,
								_pepper_xkb_handle_keyboard_key, keyboard);

	xkb->listener_keymap_update = pepper_object_add_event_listener((pepper_object_t *)keyboard,
								PEPPER_EVENT_KEYBOARD_KEYMAP_UPDATE, 0,
								_pepper_xkb_handle_keymap_update, keyboard);

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

PEPPER_API struct xkb_keymap *
pepper_xkb_get_keymap(pepper_xkb_t *xkb)
{
	PEPPER_CHECK(xkb, return NULL, "invalid xkb!\n");
	PEPPER_CHECK(xkb->info, return NULL, "keymap is not ready\n");

	return xkb->info->keymap;
}

PEPPER_API struct xkb_context *
pepper_xkb_get_context(pepper_xkb_t *xkb)
{
	PEPPER_CHECK(xkb, return NULL, "invalid xkb!\n");
	PEPPER_CHECK(xkb->info, return NULL, "keymap is not ready\n");

	return xkb->info->context;
}

PEPPER_API struct xkb_state *
pepper_xkb_get_state(pepper_xkb_t *xkb)
{
	PEPPER_CHECK(xkb, return NULL, "invalid xkb!\n");
	PEPPER_CHECK(xkb->info, return NULL, "keymap is not ready\n");

	return xkb->info->state;
}

PEPPER_API void
pepper_xkb_keyboard_set_keymap(pepper_xkb_t *xkb,
										pepper_keyboard_t *keyboard,
										pepper_xkb_rule_names_t *names)
{
	pepper_xkb_info_t *pending_xkb_info = NULL;

	/* destroy existing pending xkb info */
	pending_xkb_info = pepper_keyboard_get_pending_xkb_info(keyboard);

	if (pending_xkb_info)
		pepper_xkb_info_destroy(pending_xkb_info);

	/* create a new xkb info with given xkb rule names */
	pending_xkb_info = pepper_xkb_info_create_with_names(names);
	pepper_keyboard_set_pending_xkb_info(keyboard, pending_xkb_info);
	pepper_xkb_set_keyboard(xkb, pending_xkb_info, keyboard);
	xkb->info = pending_xkb_info;
}

PEPPER_API pepper_xkb_t *
pepper_xkb_create()
{
	pepper_xkb_t *xkb = NULL;

	xkb = calloc(1, sizeof(pepper_xkb_t));
	PEPPER_CHECK(xkb, return NULL, "[%s] Failed to allocate memory for pepper xkb...\n", __FUNCTION__);

	return xkb;
}

PEPPER_API void
pepper_xkb_destroy(pepper_xkb_t *xkb)
{
	if (!xkb)
		return;

	if (xkb->info)
	{
		pepper_xkb_info_destroy(xkb->info);
		xkb->info = NULL;
	}

	if (xkb->listener_key)
		pepper_event_listener_remove(xkb->listener_key);

	if (xkb->listener_keymap_update)
		pepper_event_listener_remove(xkb->listener_keymap_update);

	free(xkb);
}

