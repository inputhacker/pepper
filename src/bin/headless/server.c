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
#include <dlfcn.h>
#include <pepper-input-backend.h>
#include <pepper-keyrouter-wayland.h>

/* pepper object event listeners */
pepper_event_listener_t *seat_add_listener = NULL;
pepper_event_listener_t *input_device_add_listener = NULL;
pepper_event_listener_t *input_device_remove_listener = NULL;
pepper_event_listener_t *input_event_listener = NULL;
pepper_event_listener_t *keyboard_add_listener = NULL;
pepper_event_listener_t *keyboard_event_listener = NULL;

/* input/output backend handle and structure function pointer(s) */
void *input_backend_handle = NULL;
void *output_backend_handle = NULL;
headless_io_backend_func_t input_func;
headless_io_backend_func_t output_func;

static void
_handle_keyboard_add_event(pepper_event_listener_t    *listener,
						  pepper_object_t            *object,
						  uint32_t                    id,
						  void                       *info,
						  void                       *data)
{
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)info;

	PEPPER_TRACE("[%s] keyboard added\n", __FUNCTION__);

	pepper_keyboard_set_keymap_info(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP, -1, 0);
	keyboard_event_listener = pepper_object_add_event_listener((pepper_object_t *)keyboard,
									PEPPER_EVENT_KEYBOARD_KEY, 0,
									pepper_keyrouter_wl_event_handler,
									pepper_keyboard_get_seat(keyboard));
}

static void
seat_add_callback(pepper_event_listener_t    *listener,
				  pepper_object_t            *object,
				  uint32_t                    id,
				  void                       *info,
				  void                       *data)
{
	pepper_seat_t *event = info;

	PEPPER_TRACE("[%s] seat added : %s, %p\n", __FUNCTION__, pepper_seat_get_name(event), event);

	keyboard_add_listener = pepper_object_add_event_listener((pepper_object_t *)event,
									PEPPER_EVENT_SEAT_KEYBOARD_ADD,
									0,
									_handle_keyboard_add_event, data);
}

static void
input_device_add_callback(pepper_event_listener_t    *listener,
						  pepper_object_t            *object,
						  uint32_t                    id,
						  void                       *info,
						  void                       *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;
	pepper_headless_t *headless = (pepper_headless_t *)data;

	if (headless->seat)
		pepper_seat_add_input_device(headless->seat, device);
}

static void
input_device_remove_callback(pepper_event_listener_t    *listener,
						  pepper_object_t            *object,
						  uint32_t                    id,
						  void                       *info,
						  void                       *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;
	pepper_headless_t *headless = (pepper_headless_t *)data;

	if (headless->seat)
		pepper_seat_remove_input_device(headless->seat, device);
}

static pepper_compositor_t *
headless_display_create()
{
	const char *socket_name = NULL;
	pepper_compositor_t *compositor = NULL;

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "wayland-0";

	compositor = pepper_compositor_create(socket_name);

	return compositor;
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
	handle = NULL;
}

static int
headless_input_init(pepper_headless_t *headless)
{
	pepper_compositor_t *compositor = headless->compositor;

	seat_add_listener = pepper_object_add_event_listener((pepper_object_t *)compositor,
			PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
			0, seat_add_callback, headless);

	input_device_add_listener = pepper_object_add_event_listener((pepper_object_t *)compositor,
			PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
			0, input_device_add_callback, headless);

	input_device_remove_listener = pepper_object_add_event_listener((pepper_object_t *)compositor,
			PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,
			0, input_device_remove_callback, headless);

	headless->seat = pepper_compositor_add_seat(compositor, "seat0");
	PEPPER_CHECK(headless->seat, return -1, "Failed to pepper_compositor_add_seat()...");

	uint32_t caps = 0;
	if (getenv("HEADLESS_INPUT_KEYBOARD"))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (getenv("HEADLESS_INPUT_POINTER"))
		caps |= WL_SEAT_CAPABILITY_POINTER;
	if (getenv("HEADLESS_INPUT_TOUCH"))
		caps |= WL_SEAT_CAPABILITY_TOUCH;
	if (!caps)
		caps = WL_SEAT_CAPABILITY_KEYBOARD;

	headless->input_device = pepper_input_device_create(compositor, caps, NULL, NULL);
	PEPPER_CHECK(headless->input_device, return -1, "Failed on pepper_input_device_create()...");

	/* pepper keyrouter initialization */
	pepper_bool_t res = pepper_keyrouter_wl_init(compositor);

	if (res == PEPPER_FALSE)
	{
		PEPPER_ERROR("Failed on initializing pepper keyrouter...\n");
		return -1;
	}

	/* headless input module (backend) init */
	input_backend_handle = load_io_module("headless-input-backend.so",
											"headless_input_backend",
											&input_func);

	if (!input_backend_handle)
	{
		PEPPER_ERROR("Failed on loading input module...");
		return 0;
	}

	input_func.backend_init(headless);

	return 1;
}

static int
headless_output_init(pepper_headless_t *headless)
{
	/* headless output module (backend) init */
	output_backend_handle = load_io_module("headless-output-backend.so",
											"headless_output_backend",
											&output_func);

	if (!output_backend_handle)
	{
		PEPPER_ERROR("Failed on loading output module...");
		return 0;
	}

	headless->output_backend = output_func.backend_init(headless);
	PEPPER_CHECK(headless->output_backend, return 0, "Failed to init output backend...\n");

	return 1;
}

static int
headless_display_destroy(pepper_headless_t *headless)
{
	pepper_compositor_destroy(headless->compositor);

	return 0;
}

static int
headless_input_shutdown(pepper_headless_t *headless)
{
	unload_io_module(input_backend_handle, &input_func);

	pepper_event_listener_remove(seat_add_listener);
	pepper_event_listener_remove(input_device_add_listener);
	pepper_event_listener_remove(input_device_remove_listener);

	pepper_input_device_destroy(headless->input_device);
	pepper_seat_destroy(headless->seat);

	pepper_keyrouter_wl_deinit();

	return 0;
}

static int
headless_output_shutdown(pepper_headless_t *headless)
{
	unload_io_module(output_backend_handle, &output_func);

	return 0;
}

static void
headless_shutdown(pepper_headless_t *headless)
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	headless_output_shutdown(headless);
	headless_input_shutdown(headless);
	headless_display_destroy(headless);

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);
}

static pepper_headless_t *
headless_init()
{
	input_func.backend_init = NULL;
	input_func.backend_fini = NULL;
	output_func.backend_init = NULL;
	output_func.backend_fini = NULL;

	pepper_headless_t *headless = NULL;
	pepper_compositor_t *compositor = NULL;

	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	compositor = headless_display_create();
	PEPPER_CHECK(compositor, return 0, "Failed to create pepper compositor...\n");

	headless = pepper_headless_create(compositor);
	PEPPER_CHECK(headless, return 0, "Failed to create pepper headless...\n");

	if (0 > headless_input_init(headless))
	{
		PEPPER_ERROR("Failed on input init...\n");
		goto error;
	}

	if (0 > headless_output_init(headless))
	{
		PEPPER_ERROR("Failed on output init...\n");
		goto error;
	}

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return headless;

error:

	headless_shutdown(headless);

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);

	return NULL;
}

static void
headless_run(pepper_headless_t *headless)
{
	PEPPER_TRACE("%s -- begin\n", __FUNCTION__);

	/* Enter main loop. */
	wl_display_run(headless->display);

	PEPPER_TRACE("%s -- end\n", __FUNCTION__);
}

int
main(int argc, char **argv)
{
	pepper_headless_t *headless = NULL;

	headless = headless_init();
	PEPPER_CHECK(headless, return -1, "Failed on headless init...\n");

	headless_run(headless);
	headless_shutdown(headless);

	return 0;
}
