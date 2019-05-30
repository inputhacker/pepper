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
#include <tizen-extension-server-protocol.h>

PEPPER_API pepper_list_t *
keyrouter_grabbed_list_get(keyrouter_t *keyrouter,
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

PEPPER_API pepper_bool_t
keyrouter_is_grabbed_key(keyrouter_t *keyrouter, int keycode)
{
	pepper_list_t *list;

	PEPPER_CHECK(keyrouter, return PEPPER_FALSE, "keyrouter is invalid.\n");

	list = keyrouter_grabbed_list_get(keyrouter, TIZEN_KEYROUTER_MODE_EXCLUSIVE, keycode);
	if (list && !pepper_list_empty(list)) return PEPPER_TRUE;

	list = keyrouter_grabbed_list_get(keyrouter, TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE, keycode);
	if (list && !pepper_list_empty(list)) return PEPPER_TRUE;

	list = keyrouter_grabbed_list_get(keyrouter, TIZEN_KEYROUTER_MODE_TOPMOST, keycode);
	if (list && !pepper_list_empty(list)) return PEPPER_TRUE;

	list = keyrouter_grabbed_list_get(keyrouter, TIZEN_KEYROUTER_MODE_SHARED, keycode);
	if (list && !pepper_list_empty(list)) return PEPPER_TRUE;

	return PEPPER_FALSE;
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

	list = keyrouter_grabbed_list_get(keyrouter, type, keycode);
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

PEPPER_API void
keyrouter_set_focus_client(keyrouter_t *keyrouter, void *focus_client)
{
	PEPPER_CHECK(keyrouter, return, "keyrouter is invalid.\n");
	keyrouter->focus_client = focus_client;

	if (focus_client)
		PEPPER_TRACE("[%s] focus client has been set. (focus_client=0x%p)\n", __FUNCTION__, focus_client);
	else
		PEPPER_TRACE("[%s] focus client has been set to NULL.\n", __FUNCTION__);
}

PEPPER_API void
keyrouter_set_top_client(keyrouter_t *keyrouter, void *top_client)
{
	PEPPER_CHECK(keyrouter, return, "keyrouter is invalid.\n");

	keyrouter->top_client = top_client;

	if (top_client)
		PEPPER_TRACE("[%s] top client has been set. (top_client=0x%p)\n", __FUNCTION__, top_client);
	else
		PEPPER_TRACE("[%s] top client has been set to NULL.\n", __FUNCTION__);
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
		info = pepper_container_of(keyrouter->hard_keys[keycode].grab.excl.next, info, link);
		if (info) {
			delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
			PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
			delivery->data = info->data;
			pepper_list_insert(delivery_list, &delivery->link);
			PEPPER_TRACE("Exclusive Mode: keycode: %d to data: %p\n", keycode, info->data);
			return 1;
		}
	}
	else if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.or_excl)) {
		info = pepper_container_of(keyrouter->hard_keys[keycode].grab.or_excl.next, info, link);
		if (info) {
			delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
			PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
			delivery->data = info->data;
			pepper_list_insert(delivery_list, &delivery->link);
			PEPPER_TRACE("OR-Excl Mode: keycode: %d to data: %p\n", keycode, info->data);
			return 1;
		}
	}
	else if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.top)) {
		pepper_list_for_each(info, &keyrouter->hard_keys[keycode].grab.top, link) {
			if (keyrouter->top_client && keyrouter->top_client == info->data) {
				delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
				PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
				delivery->data = info->data;
				pepper_list_insert(delivery_list, &delivery->link);
				PEPPER_TRACE("Topmost Mode: keycode: %d to data: %p\n", keycode, info->data);
				return 1;
			}
		}
	}

	if (keyrouter->focus_client) {
		delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
		PEPPER_CHECK(delivery, return 0, "Failed to allocate memory\n");
		delivery->data = keyrouter->focus_client;
		pepper_list_insert(delivery_list, &delivery->link);
		count++;
		PEPPER_TRACE("Focus: keycode: %d to data: %p, count: %d\n", keycode, delivery->data, count);
	}

	if (!pepper_list_empty(&keyrouter->hard_keys[keycode].grab.shared)) {
		pepper_list_for_each(info, &keyrouter->hard_keys[keycode].grab.shared, link) {
			if (keyrouter->focus_client && keyrouter->focus_client == info->data)
				continue;
			delivery = (keyrouter_key_info_t *)calloc(1, sizeof(keyrouter_key_info_t));
			PEPPER_CHECK(delivery, return count, "Failed to allocate memory\n");
			delivery->data = info->data;
			pepper_list_insert(delivery_list, &delivery->link);
			count++;
			PEPPER_TRACE("Shared: keycode: %d to data: %p, count: %d\n", keycode, info->data, count);
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
	PEPPER_CHECK(info, return TIZEN_KEYROUTER_ERROR_NO_SYSTEM_RESOURCES, "Failed to allocate memory");

	info->data = data;
	pepper_list_init(&info->link);

	list = keyrouter_grabbed_list_get(keyrouter, type, keycode);

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

	list = keyrouter_grabbed_list_get(keyrouter, type, keycode);
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
