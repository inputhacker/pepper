#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <tizen-extension-client-protocol.h>
#include <protocol/tizen-headless-client-protocol.h>

#define DEBUG
#ifdef DEBUG
#define TRACE(fmt, ...)	\
    do {					\
        printf("[headless-client : %s] "fmt, __FUNCTION__, ##__VA_ARGS__);	\
    } while (0)
#else
#define TRACE(fmt, ...)
    do {                                                                                \
	;
    } while (0)
#endif

#define ERROR_CHECK(exp, action, fmt, ...)		\
    do {										\
        if (!(exp))								\
        {										\
            printf(fmt, ##__VA_ARGS__);	\
            action;								\
        }										\
    } while (0)

typedef struct tizen_keyrouter_notify tizen_keyrouter_notify_t;
struct tizen_keyrouter_notify
{
	int keycode;
	int error;
	int mode;
};

typedef struct headless_info headless_info_t;
struct headless_info
{
	struct wl_display *display;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct tizen_keyrouter *tz_keyrouter;
	struct tizen_headless *tz_headless;

	unsigned int has_keymap:1;
	unsigned int enter:1;

	struct wl_array array;
};

// wl_keyboard listener
static void keyboard_keymap(void *, struct wl_keyboard *, unsigned int, int fd, unsigned int);
static void keyboard_enter(void *, struct wl_keyboard *, unsigned int, struct wl_surface *, struct wl_array *);
static void keyboard_leave(void *, struct wl_keyboard *, unsigned int, struct wl_surface *);
static void keyboard_key(void *, struct wl_keyboard *, unsigned int, unsigned int, unsigned int, unsigned int);
static void keyboard_modifiers(void *, struct wl_keyboard *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
static void keyboard_repeat_setup(void *, struct wl_keyboard *, int32_t, int32_t);

// wl_seat listener
static void seat_handle_capabilities(void *, struct wl_seat *, enum wl_seat_capability);
static void seat_handle_name(void *data, struct wl_seat *seat, const char *name);

// wl_registry listener
static void handle_global(void *, struct wl_registry *, unsigned int, const char *, unsigned int);
static void handle_global_remove(void *, struct wl_registry *, unsigned int);

// tizen_keyrouter listener
static void keygrab_notify(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t key, uint32_t mode, uint32_t error);
static void keygrab_notify_list(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, struct wl_array *notify_result);
static void getgrab_notify_list(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, struct wl_array *notify_result);
static void set_register_none_key(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t mode);
static void keyregister_notify(void *data, struct tizen_keyrouter *tizen_keyrouter, uint32_t status);
static void set_input_config(void *data, struct tizen_keyrouter *tizen_keyrouter, uint32_t status);
static void key_cancel(void *data, struct tizen_keyrouter *tizen_keyrouter, uint32_t key);

// tizen_headless listener : none


static const struct wl_registry_listener registry_listener = {
	handle_global,
	handle_global_remove,
};

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
	seat_handle_name,
};

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_keymap,
	keyboard_enter,
	keyboard_leave,
	keyboard_key,
	keyboard_modifiers,
	keyboard_repeat_setup,
};

static const struct tizen_keyrouter_listener keyrouter_listener = {
	keygrab_notify,
	keygrab_notify_list,
	getgrab_notify_list,
	set_register_none_key,
	keyregister_notify,
	set_input_config,
	key_cancel,
};

// TODO : deal with tizen headless event(s)
//static const struct tizen_headless_listener headless_listener = {
//};

static void
keyboard_keymap(void *data, struct wl_keyboard *keyboard, unsigned int format, int fd, unsigned int size)
{
	headless_info_t *headless = (headless_info_t *)data;

	if (format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP)
	{
		headless->has_keymap = 0;
	}
	else
	{
		headless->has_keymap = 1;
		TRACE("headless-client : wl_keyboard has keymap...Now the given fd is going to be closed.\n");

		close(fd);
	}

	TRACE("has_keymap = %s\n", headless->has_keymap ? "true" : "false");
}

static void
keyboard_enter(void *data, struct wl_keyboard *keyboard, unsigned int serial, struct wl_surface *surface, struct wl_array *keys)
{
	headless_info_t *headless = (headless_info_t *)data;

	headless->enter = 1;
	TRACE("keyboard enter\n");
}

static void
keyboard_leave(void *data, struct wl_keyboard *keyboard, unsigned int serial, struct wl_surface *surface)
{
	headless_info_t *headless = (headless_info_t *)data;

	headless->enter = 0;
	TRACE("keyboard leave\n");
}

static void
keyboard_key(void *data, struct wl_keyboard *keyboard, unsigned int serial, unsigned int timestamp, unsigned int keycode, unsigned int state)
{
	headless_info_t *headless = (headless_info_t *)data;

	if (!headless->has_keymap)
	{
		//TODO : do anything with the given keycode
		//           ex> call application callback(s)
		TRACE("keycode : %d, state : %d, timestamp : %d\n", keycode, state, timestamp);
	}
	else
	{
		TRACE("keycode(%d) won't be dealt with as there is no keymap.\n", keycode);
	}
}

static void
keyboard_modifiers(void *data, struct wl_keyboard *keyboard, unsigned int serial, unsigned int depressed, unsigned int latched, unsigned int locked, unsigned int group)
{
	TRACE("...\n");
}

static void
keyboard_repeat_setup(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
	TRACE("...\n");
}

static void
seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
	headless_info_t *headless = (headless_info_t *)data;

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD))// && (!headless->keyboard))
	{
		if (headless->keyboard)
		{
			wl_keyboard_release(headless->keyboard);
			headless->keyboard = NULL;
		}

		headless->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_set_user_data(headless->keyboard, headless);
		wl_keyboard_add_listener(headless->keyboard, &keyboard_listener, headless);
		TRACE("seat caps update : keyboard added\n");
	}
	else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && (headless->keyboard))
	{
		wl_keyboard_release(headless->keyboard);
		headless->keyboard = NULL;

	}
}

static void
seat_handle_name(void *data, struct wl_seat *seat, const char *name)
{
	//TODO : handle seat name properly
}

static void
keygrab_notify(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t key, uint32_t mode, uint32_t error)
{
	TRACE("key : %d, mode : %d, error : %d\n", key, mode, error);
}

static void
keygrab_notify_list(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, struct wl_array *notify_result)
{
	tizen_keyrouter_notify_t *tz_keyrouter_notify;
	headless_info_t *headless = (headless_info_t *)data;

	wl_array_init(&headless->array);
	wl_array_copy(&headless->array, notify_result);

	wl_array_for_each(tz_keyrouter_notify, &headless->array)
	{
		TRACE("... keygrab result ! (keycode : %d, mode : %d, error : %d)\n",
				tz_keyrouter_notify->keycode, tz_keyrouter_notify->mode, tz_keyrouter_notify->error);
	}

	wl_array_release(&headless->array);
}

static void getgrab_notify_list(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, struct wl_array *notify_result)
{
	TRACE("...\n");
}

static void set_register_none_key(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t mode)
{
	TRACE("...\n");
}

static void keyregister_notify(void *data, struct tizen_keyrouter *tizen_keyrouter, uint32_t status)
{
	TRACE("...\n");
}

static void set_input_config(void *data, struct tizen_keyrouter *tizen_keyrouter, uint32_t status)
{
	TRACE("...\n");
}

static void key_cancel(void *data, struct tizen_keyrouter *tizen_keyrouter, uint32_t key)
{
	TRACE("...\n");
}

static void
handle_global(void *data, struct wl_registry *registry, unsigned int id, const char *interface, unsigned int version)
{
	headless_info_t *headless = (headless_info_t *)data;

	if (!strcmp("wl_seat", interface))
	{
		headless->seat = wl_registry_bind(registry, id, &wl_seat_interface, 4);
		wl_seat_add_listener(headless->seat, &seat_listener, headless);
		wl_seat_set_user_data(headless->seat, headless);
	}
	else if (!strcmp("tizen_keyrouter", interface))
	{
		headless->tz_keyrouter = wl_registry_bind(registry, id, &tizen_keyrouter_interface, 1);
		tizen_keyrouter_add_listener(headless->tz_keyrouter, &keyrouter_listener, headless);
		tizen_keyrouter_set_user_data(headless->tz_keyrouter, headless);
	}
	else if (!strcmp("tizen_headless", interface))
	{
		headless->tz_headless = wl_registry_bind(registry, id, &tizen_headless_interface, 1);
		//TODO : call tizen_headless_add_listener
		tizen_headless_set_user_data(headless->tz_headless, headless);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, unsigned int id)
{
	//TODO : remove/cleanup global and related information
}

int main()
{
	const char *socket_name = NULL;
	headless_info_t *headless = NULL;
	struct wl_registry *registry = NULL;

	headless = (headless_info_t *)calloc(1, sizeof(headless_info_t));
	ERROR_CHECK(headless, return 0, "Failed to allocate memory for headless info...\n");

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "wayland-0";

	headless->display = wl_display_connect(socket_name);
	ERROR_CHECK(headless->display, return 0, "Failed to connect wayland display ! (socket_name : %s)\n", socket_name);

	registry = wl_display_get_registry(headless->display);
	ERROR_CHECK(registry, return 0, "Failed to get registry...\n");
	wl_registry_add_listener(registry, &registry_listener, headless);

	wl_display_dispatch(headless->display);
	wl_display_roundtrip(headless->display);

	while (-1 != wl_display_dispatch(headless->display))
	{
		;
	}

	wl_keyboard_release(headless->keyboard);
	wl_seat_destroy(headless->seat);
	tizen_keyrouter_destroy(headless->tz_keyrouter);
	tizen_headless_destroy(headless->tz_headless);

	wl_display_disconnect(headless->display);

	free(headless);
	return 0;
}

