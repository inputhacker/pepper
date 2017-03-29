#include <pepper.h>
#include <pepper-evdev.h>
#include <pepper-keyrouter.h>
#include <stdlib.h>

static void
_seat_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_seat_t *seat = (pepper_seat_t *)info;

	PEPPER_TRACE("[%s] seat added. name:%s\n", __FUNCTION__, pepper_seat_get_name(seat));
}

static void
_input_device_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;

	PEPPER_TRACE("[%s] input device added.\n", __FUNCTION__);
}

static void
_input_device_remove(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;

	PEPPER_TRACE("[%s] input device removed.\n", __FUNCTION__);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	uint32_t caps = 0;
	uint32_t probed = 0;

	const char* socket_name = NULL;
	const char* seat_name = NULL;

	pepper_compositor_t *compositor = NULL;
	pepper_keyrouter_t *keyrouter = NULL;
	pepper_seat_t *seat = NULL;
	pepper_input_device_t *input_device = NULL;

	pepper_event_listener_t *listener_seat_add = NULL;
	pepper_event_listener_t *listener_input_add = NULL;
	pepper_event_listener_t *listener_input_remove = NULL;

	socket_name = getenv("WAYLAND_DISPLAY");

	if (socket_name)
		socket_name = "wayland-0";

	/* create pepper compositor */
	compositor = pepper_compositor_create(socket_name);
	PEPPER_CHECK(compositor, return EXIT_FAILURE, "Failed to create compositor !\n");

	/* register event handlers */
	listener_seat_add = pepper_object_add_event_listener(compositor,
						PEPPER_EVENT_COMPOSITOR_SEAT_ADD, 0, _seat_add, compositor);
	PEPPER_CHECK(listener_seat_add, goto shutdown_on_failure, "Failed to add seat add listener.\n");

	listener_input_add = pepper_object_add_event_listener(compositor,
						PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD, 0, _input_device_add, compositor);
	PEPPER_CHECK(listener_input_add, goto shutdown_on_failure, "Failed to add input device add listener.\n");

	listener_input_remove = pepper_object_add_event_listener(compositor,
						PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE, 0, _input_device_remove, compositor);
	PEPPER_CHECK(listener_input_remove, goto shutdown_on_failure, "Failed to add input device remove listener.\n");

	/* create pepper keyrouter */
	keyrouter = pepper_keyrouter_create(compositor);
	PEPPER_CHECK(keyrouter, goto shutdown_on_failure, "Failed to create keyrouter !\n");

	/* seat add */
	seat_name = getenv("WAYLAND_SEAT");

	if (!seat_name)
		seat_name = "seat0";

	seat = pepper_compositor_add_seat(compositor, seat_name);
	PEPPER_CHECK(seat, goto shutdown_on_failure, "Failed to add seat !\n");

	/* create pepper input device */
	if (getenv("WAYLAND_INPUT_KEYBOARD"))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (getenv("WAYLAND_INPUT_POINTER"))
		caps |= WL_SEAT_CAPABILITY_POINTER;
	if (getenv("WAYLAND_INPUT_TOUCH"))
		caps |= WL_SEAT_CAPABILITY_TOUCH;
	if (!caps)
		caps = WL_SEAT_CAPABILITY_KEYBOARD;

	input_device = pepper_input_device_create(compositor, caps, NULL, NULL);
	PEPPER_CHECK(input_device, goto shutdown_on_failure, "Failed to create input device !\n");

	/* create pepper evdev */
	pepper_evdev_t *evdev = NULL;
	evdev = pepper_evdev_create(compositor);
	PEPPER_CHECK(evdev, goto shutdown_on_failure, "Failed to create evdev !\n");

	/* probe evdev input device(s) */
	probed = pepper_evdev_device_probe(evdev, caps);

	if (!probed)
		PEPPER_TRACE("No evdev devices have been probed.\n");
	else
		PEPPER_TRACE("%d evdev device(s) has been probed.\n", probed);

	/* create server module(s) loader : TODO */

	/* run event loop */
	wl_display_run(pepper_compositor_get_display(compositor));

	goto shutdown;

shutdown_on_failure:
	ret = EXIT_FAILURE;
shutdown:

	/* destroy all remain event listeners */
	if (listener_seat_add)
	{
		pepper_event_listener_remove(listener_seat_add);
		listener_seat_add = NULL;
	}

	if (listener_input_add)
	{
		pepper_event_listener_remove(listener_input_add);
		listener_input_add = NULL;
	}

	if (listener_input_remove)
	{
		pepper_event_listener_remove(listener_input_remove);
		listener_input_remove = NULL;
	}

	/* destroy input device */
	if (input_device)
	{
		pepper_input_device_destroy(input_device);
		input_device = NULL;
	}

	/* destroy seat */
	if (seat)
	{
		pepper_seat_destroy(seat);
		seat = NULL;
	}

	/* TODO : clean up any other objects if needed */

	/* destroy keyrouter */
	if (keyrouter)
	{
		pepper_keyrouter_destroy(keyrouter);
		keyrouter = NULL;

	}

	/* destroy evdev */
	if (evdev)
	{
		pepper_evdev_destroy(evdev);
		evdev = NULL;
	}

	/* destroy compositor */
	if (compositor)
	{
		pepper_compositor_destroy(compositor);
		compositor = NULL;
	}

	return ret;
}
