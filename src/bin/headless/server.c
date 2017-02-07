/*
* Copyright © 2017 Samsung Electronics co., Ltd. All Rights Reserved.
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

struct wl_display   *display;
pepper_compositor_t *compositor;
pepper_seat_t *seat = NULL;
pepper_input_device_t *keyboard_input = NULL;

pepper_event_listener_t *seat_add_listener = NULL;
pepper_event_listener_t *input_device_add_listener = NULL;
pepper_event_listener_t *input_device_remove_listener = NULL;
pepper_event_listener_t *input_event_listener = NULL;

static void
seat_add_callback(pepper_event_listener_t    *listener,
				  pepper_object_t            *object,
				  uint32_t                    id,
				  void                       *info,
				  void                       *data)
{
	pepper_seat_t *event = info;
	PEPPER_TRACE("Seat is added: %s, %p\n", pepper_seat_get_name(event), event);
}

static void
_handle_keyboard_event(pepper_input_event_t *info, void *data)
{
	struct wl_resource *resource;
	pepper_keyboard_t *keyboard_info;

	keyboard_info = pepper_seat_get_keyboard(seat);

	wl_resource_for_each(resource, pepper_keyboard_get_resource_list(keyboard_info)) {
		wl_keyboard_send_key(resource, wl_display_get_serial(display), info->time, info->key, info->state);
	}
}

static void
_handle_input_event(pepper_event_listener_t *listener,
						 pepper_object_t *object,
						 uint32_t id, void *info, void *data)
{
	switch (id) {
		case PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY:
			_handle_keyboard_event((pepper_input_event_t *)info, data);
			break;
	}
}

static void
input_device_add_callback(pepper_event_listener_t    *listener,
						  pepper_object_t            *object,
						  uint32_t                    id,
						  void                       *info,
						  void                       *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;

	if (seat) pepper_seat_add_input_device(seat, device);
	input_event_listener = pepper_object_add_event_listener((pepper_object_t *)device, PEPPER_EVENT_ALL, 0, _handle_input_event, NULL);
}

static void
input_device_remove_callback(pepper_event_listener_t    *listener,
						  pepper_object_t            *object,
						  uint32_t                    id,
						  void                       *info,
						  void                       *data)
{
	//pepper_input_device_t *device = (pepper_input_device_t *)info;
}

static int
headless_display_init()
{
	compositor = pepper_compositor_create("wayland-0");
	if (!compositor)
		return -1;

	display = pepper_compositor_get_display(compositor);
	if (!display) {
		pepper_compositor_destroy(compositor);
		return -1;
	}

	return 0;
}

static int
headless_input_init()
{
	//core input

	seat_add_listener = pepper_object_add_event_listener((pepper_object_t *)compositor,
			PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
			0, seat_add_callback, NULL);

	input_device_add_listener = pepper_object_add_event_listener((pepper_object_t *)compositor,
			PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
			0, input_device_add_callback, NULL);

	input_device_remove_listener = pepper_object_add_event_listener((pepper_object_t *)compositor,
			PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,
			0, input_device_remove_callback, NULL);

	seat = pepper_compositor_add_seat(compositor, "seat0");
	PEPPER_CHECK(seat, return -1, "Failed to pepper_compositor_add_seat()...");

	uint32_t caps = 0;
	if (getenv("HEADLESS_INPUT_KEYBOARD"))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (getenv("HEADLESS_INPUT_POINTER"))
		caps |= WL_SEAT_CAPABILITY_POINTER;
	if (getenv("HEADLESS_INPUT_TOUCH"))
		caps |= WL_SEAT_CAPABILITY_TOUCH;
	if (!caps)
		caps = WL_SEAT_CAPABILITY_KEYBOARD;

	keyboard_input = pepper_input_device_create(compositor, caps, NULL, NULL);
	PEPPER_CHECK(keyboard_input, return -1, "Failed on pepper_input_device_create()...");

	//pepper_keyrouter_wl_init();

	//headless input module (backend) init
}

static int
headless_output_init()
{
	//core output


	//headless output module (backend) init
}

static int
headless_display_shutdown()
{
	pepper_compositor_destroy(compositor);
}

static int
headless_input_shutdown()
{
}

static int
headless_output_shutdown()
{
}

static void
headless_run()
{
	/* Enter main loop. */
	wl_display_run(display);
}

int
main(int argc, char **argv)
{
	headless_display_init();
	headless_input_init();
	headless_output_init();

	headless_run();

	headless_output_shutdown();
	headless_input_shutdown();
	headless_display_shutdown();

	return 0;
}
