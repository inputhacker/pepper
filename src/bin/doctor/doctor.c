#include <pepper.h>
#include <pepper-input-backend.h>
#include <pepper-evdev.h>
#include <pepper-keyrouter.h>
#include <stdlib.h>
#include <tbm_bufmgr.h>

/* basic pepper objects */
pepper_seat_t *seat = NULL;
pepper_evdev_t *evdev = NULL;
pepper_keyrouter_t *keyrouter = NULL;
pepper_compositor_t *compositor = NULL;
pepper_input_device_t *input_device = NULL;

/* tbm buffer manager */
tbm_bufmgr bufmgr;

/* event listeners */
pepper_event_listener_t *listener_seat_add = NULL;
pepper_event_listener_t *listener_input_add = NULL;
pepper_event_listener_t *listener_input_remove = NULL;
pepper_event_listener_t *listener_keyboard_add = NULL;
pepper_event_listener_t *listener_keyboard_event = NULL;
pepper_event_listener_t *listener_pointer_add = NULL;
pepper_event_listener_t *listener_pointer_event = NULL;
pepper_event_listener_t *listener_touch_add = NULL;
pepper_event_listener_t *listener_touch_event = NULL;

/* pointer event handler */
static void
_handle_pointer_event(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_event_t *event;

	PEPPER_CHECK((id >= PEPPER_EVENT_POINTER_MOTION && id <= PEPPER_EVENT_POINTER_AXIS), return, "unknown event %d !\n", id);
	PEPPER_CHECK(info, return, "Invalid event !\n");
	PEPPER_CHECK(data, return, "Invalid data !\n");

	event = (pepper_input_event_t *)info;

	/* TODO: */
	(void) event;

	switch (id)
	{
		case PEPPER_EVENT_POINTER_MOTION:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_POINTER_MOTION\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_POINTER_MOTION_ABSOLUTE:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_POINTER_MOTION_ABSOLUTE\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_POINTER_BUTTON:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_POINTER_BUTTON\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_POINTER_AXIS:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_POINTER_AXIS\n", __FUNCTION__);
			break;
		default:
			break;
	}
}

/* touch event handler */
static void
_handle_touch_event(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_event_t *event;

	PEPPER_CHECK((id >= PEPPER_EVENT_TOUCH_DOWN && id <= PEPPER_EVENT_TOUCH_CANCEL), return, "unknown event %d !\n", id);
	PEPPER_CHECK(info, return, "Invalid event !\n");
	PEPPER_CHECK(data, return, "Invalid data !\n");

	event = (pepper_input_event_t *)info;

	/* TODO: */
	(void) event;

	switch (id)
	{
		case PEPPER_EVENT_TOUCH_DOWN:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_TOUCH_DOWN\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_TOUCH_UP:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_TOUCH_UP\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_TOUCH_MOTION:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_TOUCH_MOTION\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_TOUCH_FRAME:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_TOUCH_FRAME\n", __FUNCTION__);
			break;
		case PEPPER_EVENT_TOUCH_CANCEL:
			/* TODO: */
			PEPPER_TRACE("[%s] PEPPER_EVENT_TOUCH_CANCEL\n", __FUNCTION__);
			break;
		default:
			break;
	}
}

/* seat keyboard add event handler */
static void
_handle_seat_keyboard_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)info;

	PEPPER_TRACE("[%s] keyboard added\n", __FUNCTION__);

	pepper_keyboard_set_keymap_info(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP, -1, 0);

	listener_keyboard_event = pepper_object_add_event_listener((pepper_object_t *)keyboard,
								PEPPER_EVENT_KEYBOARD_KEY,
								0,
								pepper_keyrouter_event_handler,
								(void *)keyrouter);
}

/* seat pointer add event handler */
static void
_handle_seat_pointer_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_pointer_t *pointer = (pepper_pointer_t *)info;

	PEPPER_TRACE("[%s] pointer added\n", __FUNCTION__);

	listener_pointer_event = pepper_object_add_event_listener((pepper_object_t *)pointer,
								PEPPER_EVENT_POINTER_MOTION
								| PEPPER_EVENT_POINTER_MOTION_ABSOLUTE
								| PEPPER_EVENT_POINTER_BUTTON
								| PEPPER_EVENT_POINTER_AXIS,
								0,
								_handle_pointer_event,
								/*compositor*/ data);
}

/* seat touch add event handler */
static void
_handle_seat_touch_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_touch_t *touch = (pepper_touch_t *)info;

	PEPPER_TRACE("[%s] touch added\n", __FUNCTION__);

	listener_touch_event = pepper_object_add_event_listener((pepper_object_t *)touch,
								PEPPER_EVENT_TOUCH_DOWN
								| PEPPER_EVENT_TOUCH_UP
								| PEPPER_EVENT_TOUCH_MOTION
								| PEPPER_EVENT_TOUCH_FRAME
								| PEPPER_EVENT_TOUCH_CANCEL,
								0,
								_handle_touch_event,
								/*compositor*/ data);
}

/* seat add event handler */
static void
_handle_seat_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_seat_t *seat = (pepper_seat_t *)info;

	PEPPER_TRACE("[%s] seat added. name:%s\n", __FUNCTION__, pepper_seat_get_name(seat));

	listener_keyboard_add = pepper_object_add_event_listener((pepper_object_t *)seat,
								PEPPER_EVENT_SEAT_KEYBOARD_ADD,
								0,
								_handle_seat_keyboard_add, /*compositor*/ data);

	listener_pointer_add = pepper_object_add_event_listener((pepper_object_t *)seat,
								PEPPER_EVENT_SEAT_POINTER_ADD,
								0,
								_handle_seat_pointer_add, /*compositor*/ data);

	listener_touch_add = pepper_object_add_event_listener((pepper_object_t *)seat,
								PEPPER_EVENT_SEAT_TOUCH_ADD,
								0,
								_handle_seat_touch_add, /*compositor*/ data);

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

int main(int argc, char *argv[])
{
	uint32_t caps = 0;
	uint32_t probed = 0;
	int ret = EXIT_SUCCESS;
	int res = 0;

	const char* socket_name = NULL;
	const char* seat_name = NULL;

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "headless-0";

	if (!getenv("XDG_RUNTIME_DIR"))
		setenv("XDG_RUNTIME_DIR", "/run", 1);

	/* create pepper compositor */
	compositor = pepper_compositor_create(socket_name);
	PEPPER_CHECK(compositor, return EXIT_FAILURE, "Failed to create compositor !\n");

	/* init tbm buffer manager */
	bufmgr = tbm_bufmgr_init(-1);
	PEPPER_CHECK(bufmgr, goto shutdown_on_failure, "Failed to init tbm buffer manager !\n");
	res = tbm_bufmgr_bind_native_display(bufmgr, (void *)pepper_compositor_get_display(compositor));
	PEPPER_CHECK(res, goto shutdown_on_failure, "Failed to bind native display with tbm buffer manager !\n");

	/* register event listeners */
	listener_seat_add = pepper_object_add_event_listener((pepper_object_t *)compositor,
						PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
						0, _handle_seat_add, compositor);
	PEPPER_CHECK(listener_seat_add, goto shutdown_on_failure, "Failed to add seat add listener.\n");

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

	/* seat add : if there is no seat name, set seat0 as a default seat name */
	seat_name = getenv("WAYLAND_SEAT");

	if (!seat_name)
		seat_name = "seat0";

	seat = pepper_compositor_add_seat(compositor, seat_name);
	PEPPER_CHECK(seat, goto shutdown_on_failure, "Failed to add seat !\n");

	/* get capabilities for a default pepper input device*/
	if (getenv("WAYLAND_INPUT_KEYBOARD"))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (getenv("WAYLAND_INPUT_POINTER"))
		caps |= WL_SEAT_CAPABILITY_POINTER;
	if (getenv("WAYLAND_INPUT_TOUCH"))
		caps |= WL_SEAT_CAPABILITY_TOUCH;

	/* if there is no capability, set keyboard capability by default */
	if (!caps)
		caps = WL_SEAT_CAPABILITY_KEYBOARD;

	/* create a default pepper input device */
	input_device = pepper_input_device_create(compositor, caps, NULL, NULL);
	PEPPER_CHECK(input_device, goto shutdown_on_failure, "Failed to create input device !\n");

	/* create pepper evdev */
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

	/* deinitialize tbm buffer manager */
	if (bufmgr)
	{
		tbm_bufmgr_deinit(bufmgr);
		bufmgr = NULL;
	}

	/* destroy compositor */
	if (compositor)
	{
		pepper_compositor_destroy(compositor);
		compositor = NULL;
	}

	return ret;
}
