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

#include <pepper.h>

#include <pepper-xkb.h>
#include <pepper-evdev.h>
#include <pepper-keyrouter.h>
#include <pepper-input-backend.h>
#include <pepper-devicemgr.h>

/* basic pepper objects */
pepper_xkb_t *xkb = NULL;
pepper_seat_t *seat = NULL;
pepper_evdev_t *evdev = NULL;
pepper_keyrouter_t *keyrouter = NULL;
pepper_input_device_t *input_device = NULL;
pepper_devicemgr_t *devicemgr = NULL;

/* event listeners */
pepper_event_listener_t *listener_seat_add = NULL;
pepper_event_listener_t *listener_seat_remove = NULL;
pepper_event_listener_t *listener_input_add = NULL;
pepper_event_listener_t *listener_input_remove = NULL;
pepper_event_listener_t *listener_keyboard_add = NULL;
pepper_event_listener_t *listener_keyboard_event = NULL;

static void
_handle_keyboard_key(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_event_t *event;

	PEPPER_CHECK(id == PEPPER_EVENT_KEYBOARD_KEY, return, "%d event will not be handled.\n", id);
	PEPPER_CHECK(data, return, "Invalid data.\n");

	event = (pepper_input_event_t *)info;
	event->key +=8;

	PEPPER_TRACE("[%s] keycode:%d, state=%d\n", __FUNCTION__, event->key, event->state);

	/* send key event via keyrouter key event handler */
	pepper_keyrouter_event_handler(listener, object, id, info, keyrouter);
}

/* seat keyboard add event handler */
static void
_handle_seat_keyboard_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_keyboard_t *keyboard = NULL;

	PEPPER_CHECK(id == PEPPER_EVENT_SEAT_KEYBOARD_ADD, return, "%d event will not be handled.\n", id);

	keyboard = (pepper_keyboard_t *)info;
	pepper_xkb_keyboard_set_keymap(xkb, keyboard, NULL);

	listener_keyboard_event = pepper_object_add_event_listener((pepper_object_t *)keyboard,
								PEPPER_EVENT_KEYBOARD_KEY,
								0,
								_handle_keyboard_key,
								keyboard);
}

/* seat add event handler */
static void
_handle_seat_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_seat_t *new_seat = (pepper_seat_t *)info;

	PEPPER_TRACE("[%s] seat added. name:%s\n", __FUNCTION__, pepper_seat_get_name(new_seat));

	listener_keyboard_add = pepper_object_add_event_listener((pepper_object_t *)new_seat,
								PEPPER_EVENT_SEAT_KEYBOARD_ADD,
								0,
								_handle_seat_keyboard_add, data);
}

/* seat remove event handler */
static void
_handle_seat_remove(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_seat_t *seat = (pepper_seat_t *)info;

	PEPPER_TRACE("[%s] seat removed (name=%s)\n", __FUNCTION__, pepper_seat_get_name(seat));

	/* remove devices belongs to this seat */
	if (input_device)
		pepper_seat_remove_input_device(seat, input_device);
}

/* compositor input device add event handler */
static void
_handle_input_device_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;

	PEPPER_TRACE("[%s] input device added.\n", __FUNCTION__);

	if (seat)
		pepper_seat_add_input_device(seat, device);
}

/* compositor input deviec remove event handler */
static void
_handle_input_device_remove(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;

	PEPPER_TRACE("[%s] input device removed.\n", __FUNCTION__);

	if (seat)
		pepper_seat_remove_input_device(seat, device);
}

static void
_pepper_devicemgr_keymap_add(pepper_list_t *list, const char *name, int keycode)
{
	pepper_devicemgr_keymap_data_t *data;

	data = (pepper_devicemgr_keymap_data_t *)calloc(1, sizeof(pepper_devicemgr_keymap_data_t));
	PEPPER_CHECK(data, return, "Failed to alloc memory\n");

	strncpy(data->name, name, UINPUT_MAX_NAME_SIZE);
	data->keycode = keycode;

	pepper_list_init(&data->link);
	pepper_list_insert(list, &data->link);
}

static void
_pepper_devicemgr_keymap_set(pepper_devicemgr_t *pepper_devicemgr, pepper_list_t *list)
{
	pepper_list_init(list);
	_pepper_devicemgr_keymap_add(list, "XF86Back", 166);
	_pepper_devicemgr_keymap_add(list, "XF86Home", 147);
	_pepper_devicemgr_keymap_add(list, "XF86Menu", 177);

	pepper_devicemgr_keymap_set(pepper_devicemgr, list);
}

int
main(int argc, char **argv)
{
	uint32_t caps = 0;
	uint32_t probed = 0;
	struct wl_display   *display;
	pepper_compositor_t *compositor;
	const char* socket_name = NULL;
	pepper_list_t keymap_list;

	if (!getenv("XDG_RUNTIME_DIR"))
		setenv("XDG_RUNTIME_DIR", "/run", 1);

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "wayland-0";

	compositor = pepper_compositor_create(socket_name);

	if (!compositor)
		return -1;

	display = pepper_compositor_get_display(compositor);

	if (!display)
	{
		pepper_compositor_destroy(compositor);
		return -1;
	}

	/* create pepper xkb */
	xkb = pepper_xkb_create();
	PEPPER_CHECK(xkb, goto shutdown_on_failure, "Failed to create pepper_xkb !\n");

	/* register event listeners */
	listener_seat_add = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
						0, _handle_seat_add, compositor);
	PEPPER_CHECK(listener_seat_add, goto shutdown_on_failure, "Failed to add seat add listener.\n");

	listener_seat_remove = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
						0, _handle_seat_remove, compositor);
	PEPPER_CHECK(listener_seat_add, goto shutdown_on_failure, "Failed to add seat remove listener.\n");

	listener_input_add = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
						0, _handle_input_device_add, compositor);
	PEPPER_CHECK(listener_input_add, goto shutdown_on_failure, "Failed to add input device add listener.\n");

	listener_input_remove = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,
						0, _handle_input_device_remove, compositor);
	PEPPER_CHECK(listener_input_remove, goto shutdown_on_failure, "Failed to add input device remove listener.\n");

	/* create pepper keyrouter */
	keyrouter = pepper_keyrouter_create(compositor);
	PEPPER_CHECK(keyrouter, goto shutdown_on_failure, "Failed to create keyrouter !\n");

	/* create pepper evdev */
	evdev = pepper_evdev_create(compositor);
	PEPPER_CHECK(evdev, goto shutdown_on_failure, "Failed to create evdev !\n");

	/* set seat0 as a default seat name */
	seat = pepper_compositor_add_seat(compositor, "seat0");
	PEPPER_CHECK(seat, goto shutdown_on_failure, "Failed to add seat !\n");

	/* create pepper devicemgr */
	devicemgr = pepper_devicemgr_create(compositor, seat);
	PEPPER_CHECK(devicemgr, goto shutdown_on_failure, "Failed to create devicemgr !\n");

	_pepper_devicemgr_keymap_set(devicemgr, &keymap_list);

	/* set keyboard capability by default */
	caps = WL_SEAT_CAPABILITY_KEYBOARD;

	/* create a default pepper input device */
	input_device = pepper_input_device_create(compositor, caps, NULL, NULL);
	PEPPER_CHECK(input_device, goto shutdown_on_failure, "Failed to create input device !\n");

	/* probe evdev input device(s) */
	probed = pepper_evdev_device_probe(evdev, caps);

	if (!probed)
		PEPPER_TRACE("No evdev devices have been probed.\n");

	/* Enter main loop. */
	wl_display_run(display);

shutdown_on_failure:

	if (xkb)
		pepper_xkb_destroy(xkb);

	if(listener_seat_add)
		pepper_event_listener_remove(listener_seat_add);
	if (listener_seat_remove)
		pepper_event_listener_remove(listener_seat_remove);
	if (listener_input_add)
		pepper_event_listener_remove(listener_input_add);
	if (listener_input_remove)
		pepper_event_listener_remove(listener_input_remove);

	if (keyrouter)
		pepper_keyrouter_destroy(keyrouter);
	if (devicemgr)
		pepper_devicemgr_destroy(devicemgr);
	if (evdev)
		pepper_evdev_destroy(evdev);
	if (input_device)
		pepper_input_device_destroy(input_device);
	if (seat)
		pepper_seat_destroy(seat);
	if (compositor)
		pepper_compositor_destroy(compositor);

	return 0;
}
