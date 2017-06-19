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

#include <libudev.h>
#include <pepper-libinput.h>
#include <pepper-desktop-shell.h>

int
main(int argc, char **argv)
{
	struct udev *udev = NULL;
	pepper_libinput_t *input = NULL;

	struct wl_display   *display;
	pepper_compositor_t *compositor;
	const char* socket_name = NULL;

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

	udev = udev_new();
	PEPPER_CHECK(udev, goto shutdown_on_failure, "Failed to get udev !\n");

	input = pepper_libinput_create(compositor, udev);
	PEPPER_CHECK(input, goto shutdown_on_failure, "Failed to create pepepr libinput !\n");

	if (!pepper_desktop_shell_init(compositor))
	{
		PEPPER_ERROR("Failed to initialize pepper desktop shell !\n");
		goto shutdown_on_failure;
	}

	/* Enter main loop. */
	wl_display_run(display);

shutdown_on_failure:

	if (input)
		pepper_libinput_destroy(input);

	if (udev)
		udev_unref(udev);

	if (compositor)
		pepper_compositor_destroy(compositor);

	return 0;
}
