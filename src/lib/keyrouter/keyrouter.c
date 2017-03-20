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

#include "keyrouter-internal.h"

static pepper_list_t *
_keyrouter_grabbed_list_get(keyrouter_t *keyrouter,
                                   int type,
                                   int keycode)
{
	PEPPER_CHECK(keyrouter, return NULL, "Invalid keyrouter\n");

	switch(type)	{
		case TIZEN_KEYROUTER_MODE_EXCLUSIVE:
			return &keyrouter->hard_keys[keycode].grab.excl;
		case TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE:
			return &keyrouter->hard_keys[keycode].grab.or_excl;
		case TIZEN_KEYROUTER_MODE_TOPMOST:
			return &keyrouter->hard_keys[keycode].grab.top;
		case TIZEN_KEYROUTER_MODE_SHARED:
			return &keyrouter->hard_keys[keycode].grab.shared;
		default:
			return NULL;
	}
}

static pepper_bool_t
_keyrouter_list_duplicated_data_check(pepper_list_t *list, void *data)
{
	keyrouter_key_info_t *info;
	PEPPER_CHECK(list, return PEPPER_FALSE, "Invalid list\n");
	if (pepper_list_empty(list)) return PEPPER_FALSE;

	pepper_list_for_each(info, list, link) {
		if (info->data == data) return PEPPER_TRUE;
	}

	return PEPPER_FALSE;
}

static pepper_bool_t
_keyrouter_grabbed_check(keyrouter_t *keyrouter,
                                int type,
                                int keycode,
                                void *data)
{
	pepper_list_t *list = NULL;
	pepper_bool_t res = PEPPER_FALSE;

	list = _keyrouter_grabbed_list_get(keyrouter, type, keycode);
	PEPPER_CHECK(list, return PEPPER_FALSE, "keycode(%d) had no list for type(%d)\n", keycode, type);

	switch(type)	{
		case TIZEN_KEYROUTER_MODE_EXCLUSIVE:
			if (!pepper_list_empty(list))
				return PEPPER_TRUE;
			break;
		case TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE:
			res = _keyrouter_list_duplicated_data_check(list, data);
			break;
		case TIZEN_KEYROUTER_MODE_TOPMOST:
			res = _keyrouter_list_duplicated_data_check(list, data);
			break;
		case TIZEN_KEYROUTER_MODE_SHARED:
			res = _keyrouter_list_duplicated_data_check(list, data);
			break;
		default:
			res = TIZEN_KEYROUTER_ERROR_INVALID_MODE;
			break;
	}
	return res;
}

PEPPER_API int
keyrouter_key_process(keyrouter_t *keyrouter,
                             int keycode, int pressed, pepper_list_t *delivery_list)
{
	keyrouter_key_info_t *info, *delivery;
	int count = 0;

	PEPPER_CHECK(keyrouter, return 0, "Invalid keyrouter\n");
	PEPPER_CHECK(0 < keycode || keycode < KEYROUTER_MAX_KEYS,
	             return 0, "Invalid keycode(%d)\n", keycode);
	PEPPER_CHECK(delivery_list, return 0, "Invalid delivery list\n");

	if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.excl)) {
		pepper_list_for_each(info, &keyrouter->hard_keys[keycode].grab.excl, link) {
			delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
			PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
			delivery->data = info->data;
			pepper_list_insert(delivery_list, &delivery->link);
			return 1;
		}
	}
	else if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.or_excl)) {
		pepper_list_for_each(info, &keyrouter->hard_keys[keycode].grab.or_excl, link) {
			delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
			PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
			delivery->data = info->data;
			pepper_list_insert(delivery_list, &delivery->link);
			return 1;
		}
	}
	else if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.top)) {
		/* Not supported yet */
		return 0;
	}
	else if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.shared)) {
		pepper_list_for_each(info, &keyrouter->hard_keys[keycode].grab.shared, link) {
			delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
			PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
			delivery->data = info->data;
			pepper_list_insert(delivery_list, &delivery->link);
			count++;
		}
	}

	return count;
}

PEPPER_API int
keyrouter_grab_key(keyrouter_t *keyrouter,
                          int type,
                          int keycode,
                          void *data)
{
	keyrouter_key_info_t *info = NULL;
	pepper_list_t *list = NULL;

	PEPPER_CHECK(keyrouter, return TIZEN_KEYROUTER_ERROR_INVALID_MODE, "Invalid keyrouter\n");
	PEPPER_CHECK(0 < keycode || keycode < KEYROUTER_MAX_KEYS,
	             return TIZEN_KEYROUTER_ERROR_INVALID_KEY, "Invalid keycode(%d)\n", keycode);

	if (_keyrouter_grabbed_check(keyrouter, type, keycode, data))
		return TIZEN_KEYROUTER_ERROR_GRABBED_ALREADY;

	info = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
	info->data = data;
	pepper_list_init(&info->link);

	list = _keyrouter_grabbed_list_get(keyrouter, type, keycode);

	if (!keyrouter->hard_keys[keycode].keycode)
		keyrouter->hard_keys[keycode].keycode = keycode;
	pepper_list_insert(list, &info->link);

	return TIZEN_KEYROUTER_ERROR_NONE;
}

static void
_keyrouter_list_remove_data(pepper_list_t *list,
                                void *data)
{
	keyrouter_key_info_t *info, *tmp;

	if (pepper_list_empty(list))
		return;

	pepper_list_for_each_safe(info ,tmp, list, link) {
		if (info->data == data) {
			pepper_list_remove(&info->link);
			free(info);
		}
	}
}

PEPPER_API void
keyrouter_ungrab_key(keyrouter_t *keyrouter,
                            int type, int keycode, void *data)
{
	pepper_list_t *list;

	PEPPER_CHECK(keyrouter, return, "Invalid keyrouter\n");
	PEPPER_CHECK(0 < keycode || keycode < KEYROUTER_MAX_KEYS,
	             return, "Invalid keycode(%d)\n", keycode);

	if (!keyrouter->hard_keys[keycode].keycode) return;

	list = _keyrouter_grabbed_list_get(keyrouter, type, keycode);
	PEPPER_CHECK(list, return, "keycode(%d) had no list for type(%d)\n", keycode, type);

	_keyrouter_list_remove_data(list, data);
}

PEPPER_API keyrouter_t *
keyrouter_create(void)
{
	keyrouter_t *keyrouter = NULL;
	int i;

	keyrouter = (keyrouter_t *)calloc(1, sizeof(keyrouter_t));
	PEPPER_CHECK(keyrouter, return NULL, "keyrouter allocation failed.\n");

	/* FIXME: Who defined max keycode? */
	keyrouter->hard_keys = (keyrouter_grabbed_t *)calloc(KEYROUTER_MAX_KEYS, sizeof(keyrouter_grabbed_t));
	PEPPER_CHECK(keyrouter->hard_keys, goto alloc_failed, "keyrouter allocation failed.\n");

	for (i = 0; i < KEYROUTER_MAX_KEYS; i++) {
		/* Enable all of keys to grab */
		//keyrouter->hard_keys[i].keycode = i;
		pepper_list_init(&keyrouter->hard_keys[i].grab.excl);
		pepper_list_init(&keyrouter->hard_keys[i].grab.or_excl);
		pepper_list_init(&keyrouter->hard_keys[i].grab.top);
		pepper_list_init(&keyrouter->hard_keys[i].grab.shared);
		pepper_list_init(&keyrouter->hard_keys[i].pressed);
	}

	return keyrouter;

alloc_failed:
	if (keyrouter) {
		if (keyrouter->hard_keys) {
			free(keyrouter->hard_keys);
			keyrouter->hard_keys = NULL;
		}
		free(keyrouter);
		keyrouter = NULL;
	}
	return NULL;
}

static void
_keyrouter_list_free(pepper_list_t *list)
{
	keyrouter_key_info_t *info, *tmp;

	if (pepper_list_empty(list)) return;

	pepper_list_for_each_safe(info, tmp, list, link) {
		pepper_list_remove(&info->link);
		free(info);
		info = NULL;
	}
}

PEPPER_API void
keyrouter_destroy(keyrouter_t *keyrouter)
{
	int i;

	PEPPER_CHECK(keyrouter, return, "Invalid keyrouter resource.\n");
	PEPPER_CHECK(keyrouter->hard_keys, return, "Invalid keyrouter resource.\n");

	for (i = 0; i < KEYROUTER_MAX_KEYS; i++) {
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.excl);
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.or_excl);
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.top);
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.shared);
		_keyrouter_list_free(&keyrouter->hard_keys[i].pressed);
	}

	free(keyrouter->hard_keys);
	keyrouter->hard_keys = NULL;
	free(keyrouter);
	keyrouter = NULL;
}
