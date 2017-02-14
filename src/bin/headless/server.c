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

#include "server.h"
#include <pepper.h>
#include <dlfcn.h>
#include <pepper-input-backend.h>
#include <pepper-keyrouter-wayland.h>

struct wl_display   *display;
pepper_compositor_t *compositor;
pepper_seat_t *seat = NULL;
pepper_input_device_t *keyboard_input = NULL;

pepper_event_listener_t *seat_add_listener = NULL;
pepper_event_listener_t *input_device_add_listener = NULL;
pepper_event_listener_t *input_device_remove_listener = NULL;
pepper_event_listener_t *input_event_listener = NULL;

void *input_backend_handle = NULL;
void *output_backend_handle = NULL;

headless_io_backend_func_t input_func;
headless_io_backend_func_t output_func;

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
	const char *socket_name = NULL;

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

	return 0;
}

static void *
load_io_module(const char *module, const char *sym_prefix, headless_io_backend_func_t *func_ptr)
{
	void *handle = NULL;
	char module_path[PATH_MAX];
	char sym[SYM_MAX];

	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

       snprintf(module_path, sizeof(module_path), "%s/%s", module_basedir, module);
	handle = dlopen(module_path, (RTLD_NOW | RTLD_GLOBAL));

	if (!handle)
	{
		PEPPER_ERROR("Failed on loading headless module : %s\n", module_path);
		func_ptr->backend_init = NULL;
		func_ptr->backend_fini = NULL;
	}

	snprintf(sym, sizeof(sym), "%s_init", sym_prefix);
	func_ptr->backend_init = dlsym(handle, sym);
	snprintf(sym, sizeof(sym), "%s_fini", sym_prefix);
	func_ptr->backend_fini = dlsym(handle, sym);

	if (!func_ptr->backend_init)
	{
		PEPPER_ERROR("Failed on getting initialization function for input backend\n");

		handle = NULL;
		func_ptr->backend_init = NULL;
		func_ptr->backend_fini = NULL;
	}

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return handle;
}

static void
unload_io_module(void *handle, headless_io_backend_func_t *func_ptr)
{
	if (!func_ptr || !handle)
		return;

	if (func_ptr->backend_fini)
		func_ptr->backend_fini(NULL);
	dlclose(handle);
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

	//Pepper keyrouter initialization
	pepper_bool_t res = pepper_keyrouter_wl_init(compositor);

	if (res == PEPPER_FALSE)
	{
		PEPPER_ERROR("Failed on initializing pepper keyrouter...\n");
		return -1;
	}

	//headless input module (backend) init
	input_backend_handle = load_io_module("headless-input-backend.so", "headless_input_backend", &input_func);

	if (!input_backend_handle)
	{
		PEPPER_ERROR("Failed on loading input module...");
		return 0;
	}

	input_func.backend_init(NULL);

	return 1;
}

static int
headless_output_init()
{
	//core output


	//headless output module (backend) init
	output_backend_handle = load_io_module("headless-output-backend.so", "headless_output_backend", &output_func);

	if (!output_backend_handle)
	{
		PEPPER_ERROR("Failed on loading output module...");
		return 0;
	}

	output_func.backend_init(NULL);

	return 1;
}

static int
headless_display_shutdown()
{
	pepper_compositor_destroy(compositor);

	return 0;
}

static int
headless_input_shutdown()
{
	unload_io_module(input_backend_handle, &input_func);

	return 0;
}

static int
headless_output_shutdown()
{
	unload_io_module(output_backend_handle, &output_func);

	return 0;
}

static int
headless_init()
{
	input_func.backend_init = NULL;
	input_func.backend_fini = NULL;
	output_func.backend_init = NULL;
	output_func.backend_fini = NULL;

	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	headless_display_init();

	if (0 > headless_input_init())
	{
		PEPPER_ERROR("Failed on input init...\n");
		return 0;
	}

	if (0 > headless_output_init())
	{
		PEPPER_ERROR("Failed on output init...\n");
		return 0;
	}

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return 1;
}

static void
headless_shutdown()
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	headless_output_shutdown();
	headless_input_shutdown();
	headless_display_shutdown();

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);
}

static void
headless_run()
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	/* Enter main loop. */
	wl_display_run(display);

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);
}

int
main(int argc, char **argv)
{
	int ret;
	ret = headless_init();

	if (!ret)
	{
		PEPPER_ERROR("Failed on headless init...\n");
		return -1;
	}

	headless_run();

	headless_shutdown();

	return 0;
}
