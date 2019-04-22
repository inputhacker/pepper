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
#include <pepper-evdev.h>
#include <pepper-input-backend.h>
#include <pepper-keyrouter.h>

typedef struct
{
	pepper_compositor_t *compositor;
	pepper_seat_t *seat;
	pepper_evdev_t *evdev;
	pepper_keyboard_t *keyboard;

	pepper_view_t *focus_view;
	pepper_view_t *top_view;

	pepper_keyrouter_t *keyrouter;

	pepper_event_listener_t *listener_seat_keyboard_key;
	pepper_event_listener_t *listener_seat_keyboard_add;
	pepper_event_listener_t *listener_seat_add;
	pepper_event_listener_t *listener_input_device_add;
	pepper_event_listener_t *listener_input_device_remove;

	uint32_t ndevices;
} headless_input_t;

headless_input_t *input = NULL;
const static int KEY_INPUT = 0xdeadbeaf;

static void init_event_listeners(headless_input_t *hi);
static void deinit_event_listeners(headless_input_t *hi);

/* PEPPER_EVENT_KEYBOARD_KEY handler (must be changed to pepper_keyrouter_event_handler) */
static void
_handle_keyboard_key(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_event_t *event;
	headless_input_t *hi = (headless_input_t *)data;
	pepper_keyboard_t *keyboard = pepper_seat_get_keyboard(hi->seat);

	event = (pepper_input_event_t *)info;
	event->key +=8;

	PEPPER_TRACE("[%s] keycode:%d, state=%d\n", __FUNCTION__, event->key, event->state);

	/* send key event to focused client */
	pepper_view_t *focus_view = pepper_keyboard_get_focus(keyboard);
	PEPPER_CHECK(focus_view, return, "[%s] No focused view exists.\n", __FUNCTION__);
	pepper_keyboard_send_key(keyboard, focus_view, event->time, event->key, event->state);
}

/* seat keyboard add event handler */
static void
_handle_seat_keyboard_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_event_listener_t *h = NULL;
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)info;
	headless_input_t *hi = (headless_input_t *)data;

	PEPPER_TRACE("[%s] keyboard added\n", __FUNCTION__);

	pepper_keyboard_set_keymap_info(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP, -1, 0);

	h = pepper_object_add_event_listener((pepper_object_t *)keyboard, PEPPER_EVENT_KEYBOARD_KEY,
								0, _handle_keyboard_key, hi);
	PEPPER_CHECK(h, goto end, "Failed to add keyboard key listener.\n");
	hi->listener_seat_keyboard_key = h;
	hi->keyboard = keyboard;

	return;

end:
	deinit_event_listeners(hi);
}

/* compositor input device add event handler */
static void
_handle_input_device_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;
	headless_input_t *hi = (headless_input_t *)data;

	/* temporary : only add keyboard device to a seat */
	if (!(WL_SEAT_CAPABILITY_KEYBOARD & pepper_input_device_get_caps(device)))
		return;

	PEPPER_TRACE("[%s] input device added.\n", __FUNCTION__);

	if (hi->seat)
		pepper_seat_add_input_device(hi->seat, device);
}

/* compositor input device remove event handler */
static void
_handle_input_device_remove(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;
	headless_input_t *hi = (headless_input_t *)data;

	/* temporary : only handle the removal of a keyboard device */
	if (!(WL_SEAT_CAPABILITY_KEYBOARD & pepper_input_device_get_caps(device)))
		return;

	PEPPER_TRACE("[%s] input device removed.\n", __FUNCTION__);

	if (hi->seat)
		pepper_seat_remove_input_device(hi->seat, device);
}

/* seat add event handler */
static void
_handle_seat_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_event_listener_t *h = NULL;
	pepper_seat_t *seat = (pepper_seat_t *)info;
	headless_input_t *hi = (headless_input_t *)data;

	PEPPER_TRACE("[%s] seat added. name:%s\n", __FUNCTION__, pepper_seat_get_name(seat));

	h = pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_KEYBOARD_ADD,
								0, _handle_seat_keyboard_add, hi);
	PEPPER_CHECK(h, goto end, "Failed to add seat keyboard add listener.\n");
	hi->listener_seat_keyboard_add = h;

	return;

end:
	deinit_event_listeners(hi);
}

void
headless_input_set_focus_view(void *input, pepper_view_t *focus_view)
{
	headless_input_t *hi = (headless_input_t *)input;

	PEPPER_CHECK(hi, return, "Invalid headless input.\n");

	if (hi->focus_view != focus_view)
	{
		if (hi->focus_view)
			pepper_keyboard_send_leave(hi->keyboard, hi->focus_view);
		pepper_keyboard_set_focus(hi->keyboard, focus_view);
		pepper_keyboard_send_enter(hi->keyboard, focus_view);
		hi->focus_view = focus_view;
	}

	if (hi->keyrouter)
		pepper_keyrouter_set_focus_view(hi->keyrouter, focus_view);
}

void
headless_input_set_top_view(void *input, pepper_view_t *top_view)
{
	headless_input_t *hi = (headless_input_t *)input;

	PEPPER_CHECK(hi, return, "Invalid headless input.\n");

	hi->top_view = top_view;

	if (hi->keyrouter)
		pepper_keyrouter_set_top_view(hi->keyrouter, top_view);
}

headless_input_t *
headless_input_get(void)
{
	return input;
}

static void
init_event_listeners(headless_input_t *hi)
{
	pepper_event_listener_t *h = NULL;
	pepper_object_t *compositor = (pepper_object_t *)hi->compositor;

	/* register event listeners */
	h = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_SEAT_ADD, 0, _handle_seat_add, hi);
	PEPPER_CHECK(h, goto end, "Failed to add seat add listener.\n");
	hi->listener_seat_add = h;

	h = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD, 0, _handle_input_device_add, hi);
	PEPPER_CHECK(h, goto end, "Failed to add input device add listener.\n");
	hi->listener_input_device_add = h;

	h = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE, 0, _handle_input_device_remove, hi);
	PEPPER_CHECK(h, goto end, "Failed to add input device remove listener.\n");
	hi->listener_input_device_remove = h;

	return;

end:
	PEPPER_ERROR("Failed to init listeners");
	deinit_event_listeners(hi);
}

static void
deinit_event_listeners(headless_input_t *hi)
{
	pepper_event_listener_remove(hi->listener_seat_keyboard_key);
	pepper_event_listener_remove(hi->listener_seat_keyboard_add);
	pepper_event_listener_remove(hi->listener_seat_add);
	pepper_event_listener_remove(hi->listener_input_device_add);
	pepper_event_listener_remove(hi->listener_input_device_remove);

	PEPPER_TRACE("[%s] event listeners have been removed.\n");
}

static void
input_deinit(headless_input_t *hi)
{
	if (hi->seat)
		pepper_seat_destroy(hi->seat);
	pepper_evdev_destroy(hi->evdev);

	hi->seat = NULL;
	hi->evdev = NULL;
	hi->ndevices = 0;
}

static pepper_bool_t
input_init(headless_input_t *hi)
{
	uint32_t caps = 0;
	uint32_t probed = 0;

	const char *seat_name = NULL;
	pepper_evdev_t *evdev = NULL;
	pepper_seat_t *seat = NULL;

	seat_name = getenv("XDG_SEAT");

	if (!seat_name)
		seat_name = "seat0";

	/* create a default seat (seat0) */
	seat = pepper_compositor_add_seat(hi->compositor, seat_name);
	PEPPER_CHECK(seat, goto end, "Failed to add seat (%s)!\n", seat_name);

	hi->seat = seat;

	/* create pepper evdev */
	evdev = pepper_evdev_create(hi->compositor);
	PEPPER_CHECK(evdev, goto end, "Failed to create evdev !\n");

	hi->evdev = evdev;

	/* probe evdev keyboard device(s) */
	caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	probed = pepper_evdev_device_probe(evdev, caps);

	if (!probed)
		PEPPER_TRACE("No evdev devices have been probed.\n");
	else
		PEPPER_TRACE("%d evdev device(s) has been probed.\n", probed);

	hi->ndevices = probed;

	return PEPPER_TRUE;

end:
	if (seat)
		pepper_seat_destroy(seat);
	pepper_evdev_destroy(evdev);

	return PEPPER_FALSE;
}

static void
init_modules(headless_input_t *hi)
{
	pepper_keyrouter_t *keyrouter = NULL;

	PEPPER_TRACE("[%s] ... begin\n", __FUNCTION);

	/* create pepper keyrouter */
	keyrouter = pepper_keyrouter_create(hi->compositor);
	PEPPER_CHECK(keyrouter, goto end, "Failed to create keyrouter !\n");

	hi->keyrouter = keyrouter;

	PEPPER_TRACE("[%s] ... done\n", __FUNCTION);

end:
	if (keyrouter)
		pepper_keyrouter_destroy(keyrouter);
}

static void
deinit_modules(headless_input_t *hi)
{
	if (hi->keyrouter)
		pepper_keyrouter_destroy(hi->keyrouter);

	hi->keyrouter = NULL;
}

static void
headless_input_deinit(void *data)
{
	headless_input_t *hi = (headless_input_t*)data;

	if (!hi) return;

	deinit_event_listeners(hi);
	deinit_modules(hi);
	input_deinit(hi);

	pepper_object_set_user_data((pepper_object_t *)hi->compositor, &KEY_INPUT, NULL, NULL);
	free(hi);

	input = hi = NULL;
}

pepper_bool_t
headless_input_init(pepper_compositor_t *compositor)
{
	headless_input_t *hi = NULL;
	pepper_bool_t init = PEPPER_FALSE;

	hi = (headless_input_t*)calloc(1, sizeof(headless_input_t));
	PEPPER_CHECK(hi, goto error, "Failed to alloc for input\n");
	hi->compositor = compositor;

	init_event_listeners(hi);
	init_modules(hi);
	init = input_init(hi);
	PEPPER_CHECK(init, goto error, "input_init() failed\n");

	input = hi;
	pepper_object_set_user_data((pepper_object_t *)compositor, &KEY_INPUT, NULL, headless_input_deinit);

	return PEPPER_TRUE;

error:
	headless_input_deinit(hi);

	return PEPPER_FALSE;
}
