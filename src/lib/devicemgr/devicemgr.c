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

#include "devicemgr-internal.h"
#include <tizen-extension-server-protocol.h>

static void
_devicemgr_generate_key(pepper_input_device_t *device, int keycode, int pressed)
{
	pepper_input_event_t event;
	struct timeval time;
	unsigned int timestamp;

	gettimeofday(&time, NULL);
	timestamp = time.tv_sec * 1000 + time.tv_usec / 1000;

	event.time = timestamp;
	event.key = keycode - 8;
	event.state = pressed ? PEPPER_KEY_STATE_PRESSED : PEPPER_KEY_STATE_RELEASED;

	pepper_object_emit_event((pepper_object_t *)device,
		PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY, &event);
}

static void
_devicemgr_update_pressed_keys(devicemgr_t *devicemgr, int keycode, pepper_bool_t pressed)
{
	devicemgr_key_t *keydata, *tmp;

	if (pressed) {
		keydata = (devicemgr_key_t *)calloc(sizeof(devicemgr_key_t), 1);
		PEPPER_CHECK(keydata, return, "Failed to alloc keydata memory.\n");
		keydata->keycode = keycode;
		pepper_list_init(&keydata->link);
		pepper_list_insert(&devicemgr->pressed_keys, &keydata->link);
	}
	else {
		pepper_list_for_each_safe(keydata, tmp, &devicemgr->pressed_keys, link) {
			if (keydata->keycode == keycode) {
				pepper_list_remove(&keydata->link);
				free(keydata);
				break;
			}
		}
	}
}

static void
_devicemgr_cleanup_pressed_keys(devicemgr_t *devicemgr)
{
	devicemgr_key_t *keydata, *tmp_keydata;

	pepper_list_for_each_safe(keydata, tmp_keydata, &devicemgr->pressed_keys, link) {
		if (devicemgr->keyboard)
			_devicemgr_generate_key(devicemgr->keyboard->input_device, keydata->keycode, PEPPER_FALSE);
		pepper_list_remove(&keydata->link);
		free(keydata);
	}
}

PEPPER_API int
devicemgr_input_generator_generate_key(devicemgr_t *devicemgr, int keycode, pepper_bool_t pressed)
{
	PEPPER_CHECK(devicemgr,
		return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES,
		"Invalid devicemgr structure.\n");
	PEPPER_CHECK(devicemgr->keyboard && devicemgr->keyboard->input_device,
		return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES,
		"Keyboard device is not initialized\n");

	_devicemgr_generate_key(devicemgr->keyboard->input_device, keycode, pressed);
	_devicemgr_update_pressed_keys(devicemgr, keycode, pressed);

	return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE;
}

PEPPER_API void
devicemgr_input_generator_keyboard_set(devicemgr_t *devicemgr, pepper_input_device_t *device)
{
	PEPPER_CHECK(devicemgr, return, "Invalid devicemgr structure.\n");
	PEPPER_CHECK(devicemgr->keyboard, return, "input generator is not initialized yet.\n");
	if (devicemgr->keyboard->input_device) return;
	devicemgr->keyboard->input_device = device;
}

PEPPER_API void
devicemgr_input_generator_keyboard_unset(devicemgr_t *devicemgr, pepper_input_device_t *device)
{
	PEPPER_CHECK(devicemgr, return, "Invalid devicemgr structure.\n");
	devicemgr->keyboard->input_device = NULL;
}


static pepper_bool_t
_devicemgr_input_generator_keyboard_create(devicemgr_t *devicemgr, const char *name)
{
	if (devicemgr->keyboard->input_device) return PEPPER_TRUE;

	devicemgr->keyboard->input_device = pepper_input_device_create(devicemgr->compositor, WL_SEAT_CAPABILITY_KEYBOARD, NULL, NULL);
	PEPPER_CHECK(devicemgr->keyboard->input_device, return PEPPER_FALSE, "Failed to create input device !\n");

	devicemgr->keyboard->created = PEPPER_TRUE;

	return PEPPER_TRUE;
}

PEPPER_API int
devicemgr_input_generator_init(devicemgr_t *devicemgr, unsigned int clas, const char *name)
{
	int ret;

	PEPPER_CHECK(devicemgr, return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES, "Invalid devicemgr structure.\n");

	if (strlen(devicemgr->keyboard->name) > 0) return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE;

	ret = _devicemgr_input_generator_keyboard_create(devicemgr, name);
	PEPPER_CHECK(ret == PEPPER_TRUE, return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES, "Failed to create keyboard device: %s\n", name);
	strncpy(devicemgr->keyboard->name, name, UINPUT_MAX_NAME_SIZE);

	return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE;
}

static void
_devicemgr_input_generator_keyboard_close(devicemgr_t *devicemgr)
{
	if (!devicemgr->keyboard->input_device) return;
	pepper_input_device_destroy(devicemgr->keyboard->input_device);
	devicemgr->keyboard->input_device = NULL;
	devicemgr->keyboard->created = PEPPER_FALSE;
}

PEPPER_API int
devicemgr_input_generator_deinit(devicemgr_t *devicemgr)
{
	PEPPER_CHECK(devicemgr, return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES, "Invalid devicemgr structure.\n");

	_devicemgr_cleanup_pressed_keys(devicemgr);

	if (!devicemgr->keyboard) return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE;

	if (devicemgr->keyboard->created)
		_devicemgr_input_generator_keyboard_close(devicemgr);
	memset(devicemgr->keyboard->name, 0, UINPUT_MAX_NAME_SIZE);

	return TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE;
}

PEPPER_API devicemgr_t *
devicemgr_create(pepper_compositor_t *compositor, pepper_seat_t *seat)
{
	devicemgr_t *devicemgr = NULL;

	PEPPER_CHECK(seat, return NULL, "Invalid seat\n");

	devicemgr = (devicemgr_t *)calloc(1, sizeof(devicemgr_t));
	PEPPER_CHECK(devicemgr, return NULL, "devicemgr allocation failed.\n");

	devicemgr->keyboard = (devicemgr_device_t *)calloc(1, sizeof(devicemgr_device_t));
	PEPPER_CHECK(devicemgr->keyboard, goto failed, "Failed to allocate device");

	devicemgr->compositor = compositor;
	devicemgr->seat = seat;

	pepper_list_init(&devicemgr->pressed_keys);

	return devicemgr;

failed:
	if (devicemgr) free(devicemgr);
	return NULL;
}

PEPPER_API void
devicemgr_destroy(devicemgr_t *devicemgr)
{
	PEPPER_CHECK(devicemgr, return, "Invalid devicemgr resource.\n");

	_devicemgr_cleanup_pressed_keys(devicemgr);

	if (devicemgr->keyboard) {
		if (devicemgr->keyboard->created)
			_devicemgr_input_generator_keyboard_close(devicemgr);

		free(devicemgr->keyboard);
		devicemgr->keyboard = NULL;
	}

	free(devicemgr);
	devicemgr = NULL;
}
