#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Ecore_Wl2.h>
#include <Ecore_Input.h>
#include <wayland-tbm-client.h>
#include <tbm_surface_internal.h>
#include <efl_util.h>

#define MAX_STR 1024

#define DEBUG
#ifdef DEBUG
#define TRACE(fmt, ...)	\
	do { \
		printf("[CLIENT PID:%u][%s] "fmt, _getpid(), __FUNCTION__, ##__VA_ARGS__); \
	} while (0)
#else
#define TRACE(fmt, ...) \
	do { \
		;
	} while (0)
#endif

#define ERROR_CHECK(exp, action, fmt, ...) \
	do { \
		if (!(exp)) \
		{ \
			printf(fmt, ##__VA_ARGS__);	\
			action; \
		} \
	} while (0)

typedef struct app_data app_data_t;
typedef enum keygrab_type keygrab_type_e;
typedef enum key_type key_type_e;

struct app_data
{
	Ecore_Wl2_Display *ewd;
	Ecore_Wl2_Window *win;

	struct wayland_tbm_client *wl_tbm_client;
	tbm_surface_queue_h tbm_queue;
	int last_serial;

	efl_util_inputgen_h inputgen;
};

enum keygrab_type
{
	KEYGRAB_TYPE_NONE,
	KEYGRAB_TYPE_SHARED,
	KEYGRAB_TYPE_TOPMOST,
	KEYGRAB_TYPE_OREXCLUSIVE,
	KEYGRAB_TYPE_EXCLUSIVE
};

enum key_type
{
   KEY_UP = 0,
   KEY_DOWN,
   KEY_ALL
};

static Eina_Array *_ecore_event_hdls;

static int KEY_WL_BUFFER = 0;
static int KEY_CLIENT = 0;

static uint32_t _getpid()
{
	static pid_t pid = 0;

	if (pid)
	{
		return pid;
	}

	pid = getpid();

	return (uint32_t)pid;
}

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	tbm_surface_h surface = (tbm_surface_h)data;
	app_data_t *client;

	tbm_surface_internal_get_user_data(surface, (unsigned long)&KEY_CLIENT, (void **)&client);
	tbm_surface_queue_release(client->tbm_queue, surface);

	//TRACE("[UPDATE] release wl_buffer:%p, surface:%p\n", buffer, surface);
}

static const struct wl_buffer_listener buffer_listener = {
    buffer_release
};

static char *
_eflutil_error_to_string(int type)
{
	switch (type) {
		case EFL_UTIL_ERROR_NONE:
			return "none";
		case EFL_UTIL_ERROR_INVALID_PARAMETER:
			return "invalid parameter";
		case EFL_UTIL_ERROR_OUT_OF_MEMORY:
			return "out of memory";
		case EFL_UTIL_ERROR_PERMISSION_DENIED:
			return "permission denied";
		case EFL_UTIL_ERROR_NO_SUCH_DEVICE:
			return "no such device";
		case EFL_UTIL_ERROR_INVALID_OPERATION:
			return "invalid operation";
		case EFL_UTIL_ERROR_NOT_SUPPORTED:
			return "not supported";
		case EFL_UTIL_ERROR_NOT_SUPPORTED_WINDOW_TYPE:
			return "not supported window type";
		case EFL_UTIL_ERROR_SCREENSHOT_INIT_FAIL:
			return "screenshot init fail";
		case EFL_UTIL_ERROR_SCREENSHOT_EXECUTION_FAIL:
			return "screenshot execution fail";
		case EFL_UTIL_ERROR_NO_RESOURCE_AVAILABLE:
			return "no resource available";
		default:
			return "Unknown";
	}
}

static void
_update_window(app_data_t *client, const char *color)
{
	struct wl_buffer *wl_buffer = NULL;
	tbm_surface_h surface;
	tbm_surface_error_e ret;

	if (color && !strncmp(color, "null", strlen("null"))) {
		goto end;
	}

	ERROR_CHECK(tbm_surface_queue_can_dequeue(client->tbm_queue, 0), return, "[UPDATE] Cannot dequeue\n");

	ret = tbm_surface_queue_dequeue(client->tbm_queue, &surface);
	ERROR_CHECK(ret == TBM_SURFACE_ERROR_NONE, return, "[UPDATE] dequeue err:%d\n", ret);

	/*TODO : Update something*/
	{
		int i;
		tbm_surface_info_s info;

		tbm_surface_map(surface, TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE, &info);

		for(i = 0; i<12; i++) {
			info.planes[0].ptr[4*i] = 0;
			info.planes[0].ptr[4*i + 1] = (((client->last_serial%3) == 0) ? 0xFF:0); /*BLUE*/
			info.planes[0].ptr[4*i + 2] = (((client->last_serial%3) == 1) ? 0xFF:0); /*GREEN*/
			info.planes[0].ptr[4*i + 3] = (((client->last_serial%3) == 2) ? 0xFF:0); /*RED*/
		}

		tbm_surface_unmap(surface);
		client->last_serial++;
	}

	ret = tbm_surface_queue_enqueue(client->tbm_queue, surface);
	ERROR_CHECK(ret == TBM_SURFACE_ERROR_NONE, return, "[UPDATE] enqueue err:%d\n", ret);

	ret = tbm_surface_queue_acquire(client->tbm_queue, &surface);
	ERROR_CHECK(ret == TBM_SURFACE_ERROR_NONE, return, "[UPDATE] acquire err:%d\n", ret);

	if (!tbm_surface_internal_get_user_data(surface, (unsigned long)&KEY_WL_BUFFER, (void **)&wl_buffer)) {
		wl_buffer = wayland_tbm_client_create_buffer(client->wl_tbm_client, surface);
		ERROR_CHECK(wl_buffer, return, "[UPDATE] failed to create wl_buffer tbm_surface:%p\n", surface);

		wl_buffer_add_listener(wl_buffer, &buffer_listener, surface);

		tbm_surface_internal_add_user_data(surface, (unsigned long)&KEY_WL_BUFFER, NULL);
		tbm_surface_internal_set_user_data(surface, (unsigned long)&KEY_WL_BUFFER, wl_buffer);
		tbm_surface_internal_add_user_data(surface, (unsigned long)&KEY_CLIENT, NULL);
		tbm_surface_internal_set_user_data(surface, (unsigned long)&KEY_CLIENT, client);
	}
	ERROR_CHECK(wl_buffer, return, "[UPDATE] dequeue err:%d\n", ret);

end:
	ecore_wl2_window_buffer_attach(client->win, wl_buffer, 0, 0, 0);
	ecore_wl2_window_damage(client->win, NULL, 0);
	ecore_wl2_window_commit(client->win, EINA_TRUE);
	//TRACE("[UPDATE] commit wl_buffer:%p, surface:%p\n", wl_buffer, surface);
}

static void
do_action(app_data_t *client, const char *keyname)
{
	if (!strncmp("XF86Back", keyname, 8) || !strncmp("XF86AudioForward", keyname, 16))
	{
		ecore_wl2_window_hide(client->win);
	}
	else if (!strncmp("XF86AudioRaiseVolume", keyname, 20))
	{
		ecore_wl2_window_focus_skip_set(client->win, EINA_FALSE);
	}
	else if (!strncmp("XF86AudioLowerVolume", keyname, 20))
	{
		ecore_wl2_window_focus_skip_set(client->win, EINA_TRUE);
	}
	else if (!strncmp("XF86Menu", keyname, 20))
	{
		_update_window(client, NULL);
	}
}

static Eina_Bool
_cb_focus_in(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Focus_In *ev = (Ecore_Wl2_Event_Focus_In *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_focus_out(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Focus_Out *ev = (Ecore_Wl2_Event_Focus_Out *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_show(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Show *ev = (Ecore_Wl2_Event_Window_Show *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_hide(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Hide *ev = (Ecore_Wl2_Event_Window_Hide *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_lower(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Lower *ev = (Ecore_Wl2_Event_Window_Lower *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_activate(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Activate *ev = (Ecore_Wl2_Event_Window_Activate *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_deactivate(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Deactivate *ev = (Ecore_Wl2_Event_Window_Deactivate *)event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_key_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Event_Key *ev = event;

	TRACE("\n");

	/* TODO */
	(void) client;
	(void) ev;

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_key_up(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Event_Key *ev = event;

	TRACE("KEY: name:%s, sym:%s, code:%d\n", ev->keyname, ev->key, ev->keycode);

	do_action(client, ev->keyname);

	return ECORE_CALLBACK_PASS_ON;
}

static char *
_keygrab_mode_to_string(keygrab_type_e type)
{
	switch (type) {
		case KEYGRAB_TYPE_SHARED:
			return "Shared";
		case KEYGRAB_TYPE_TOPMOST:
			return "Top position";
		case KEYGRAB_TYPE_OREXCLUSIVE:
			return "OR-Exclusive";
		case KEYGRAB_TYPE_EXCLUSIVE:
			return "Exclusive";
		default:
			return "Unknown";
	}
}

static void
_keygrab(char *keyname, keygrab_type_e type)
{
	Ecore_Wl2_Window_Keygrab_Mode mode;
	Eina_Bool ret;

	switch (type) {
		case KEYGRAB_TYPE_SHARED:
			mode = ECORE_WL2_WINDOW_KEYGRAB_SHARED;
			break;
		case KEYGRAB_TYPE_TOPMOST:
			mode = ECORE_WL2_WINDOW_KEYGRAB_TOPMOST;
			break;
		case KEYGRAB_TYPE_OREXCLUSIVE:
			mode = ECORE_WL2_WINDOW_KEYGRAB_OVERRIDE_EXCLUSIVE;
			break;
		case KEYGRAB_TYPE_EXCLUSIVE:
			mode = ECORE_WL2_WINDOW_KEYGRAB_EXCLUSIVE;
			break;
		default:
			printf("Input correct grab mode(%d)\n", type);
			return;
	}
	ret = ecore_wl2_window_keygrab_set(NULL, keyname, 0, 0, 0, mode);
	if (!ret) printf("Failed to grab %s to %s mode\n", keyname, _keygrab_mode_to_string(type));
	else printf("Success to grab %s to %s mode\n", keyname, _keygrab_mode_to_string(type));
}

static void
_keyungrab(char *keyname)
{
	Eina_Bool ret;

	ret = ecore_wl2_window_keygrab_unset(NULL, keyname, 0, 0);
	if (!ret) printf("Failed to ungrab %s\n", keyname);
	else printf("Success to ungrab %s\n", keyname);
}

static void
_inputgen_init(app_data_t *client)
{
	int ret = EFL_UTIL_ERROR_NONE;

	if (client->inputgen) {
		printf("Input generator is already initialized\n");
		return;
	}

	client->inputgen = efl_util_input_initialize_generator(EFL_UTIL_INPUT_DEVTYPE_KEYBOARD);
	if (!client->inputgen) {
		ret = get_last_result();
		printf("Failed to initialize input generator.(result: %s)\n", _eflutil_error_to_string(ret));
	}
}

static void
_inputgen_deinit(app_data_t *client)
{
	int ret = EFL_UTIL_ERROR_NONE;

	if (!client->inputgen) {
		printf("Input generator is not initialized\n");
		return;
	}

	ret = efl_util_input_deinitialize_generator(client->inputgen);
	if (ret != EFL_UTIL_ERROR_NONE) {
		printf("Failed to deinitialize input generator.(result: %s)\n", _eflutil_error_to_string(ret));
		return;
	}
	client->inputgen = NULL;
}

static void
_inputgen_key(app_data_t *client, char *name, int type)
{
	int ret = EFL_UTIL_ERROR_NONE;
	ret =  efl_util_input_generate_key(client->inputgen, name, type);
	if (ret != EFL_UTIL_ERROR_NONE) printf("Failed to generate key %s(result: %s)\n", type?"down":"up", _eflutil_error_to_string(ret));
}


static void
_key_gen(app_data_t *client, char *name, int type)
{
	printf("name: %s, type: %d\n", name, type);

	if (!client->inputgen) {
		printf("Input generator is not initialized\n");
		return;
	}
	if (!name) {
		printf("Invalid key name\n");
		return;
	}

	if (type == KEY_ALL) {
		_inputgen_key(client, name, 1);
		_inputgen_key(client, name, 0);
	}
	else {
		_inputgen_key(client, name, !!type);
	}
}

static void
_usage(void)
{
	printf("  Supported commands:  grab     (Grab a key)\n");
	printf("                    :  ungrab   (Ungrab a key)\n");
	printf("                    :  hide     (Hide current window)\n");
	printf("                    :  show     (Show current window)\n");
	printf("                    :  help     (Print this help text)\n");
	printf("                    :  update   (update current window)\n");
	printf("                    :  inputgen (input Generator)\n");
	printf("                    :  q/quit   (Quit program)\n");
	printf("grab [key_name] {grab_type}\n");
	printf("  : grab_type:\n");
	printf("    - default: shared\n");
	printf("    - 1: shared mode\n");
	printf("    - 2: top position mode\n");
	printf("    - 3: or-exclusive mode\n");
	printf("    - 4: exclusive mode\n");
	printf("  : ex> grab XF86Back / grab XF86Back 4\n");
	printf("ungrab [key_name]\n");
	printf("  : ex> ungrab XF86Back\n");
	printf("\n");
	printf("inputgen [options...]\n");
	printf("  : options:\n");
	printf("    - init: init generator\n");
	printf("    - deinit: deinit generator\n");
	printf("    - key: generate key event\n");
	printf("      inputgen key [keyname] {type}\n");
	printf("        - keyname: keyname (ex> XF86Back)\n");
	printf("        - type: 1: press / 0: release (default: press/release)\n");
	printf("  : ex> inputgen init\n");
	printf("  : ex> inputgen key XF86Back / inputgen key XF86Back 1\n");
	printf("  : ex> inputgen deinit\n");
	printf("\n");

	printf("* Actions *\n");

	printf("[TM1]...\n");
	printf("- hide window : XF86Back\n");
	printf("- focus skip unset : XF86AudioRaiseVolume\n");
	printf("- focus skip set : XF86AudioLowerVolume\n");

	printf("[RPi3]...\n");
	printf("- hide window : XF86AudioForward\n");
	printf("- focus skip unset : XF86AudioRaiseVolume\n");
	printf("- focus skip set : XF86AudioLowerVolume\n");

	printf("\n");
}

static Eina_Bool
_stdin_cb(void *data, Ecore_Fd_Handler *handler EINA_UNUSED)
{
	app_data_t *client = (app_data_t *)data;
	char c, buf[MAX_STR] = {0, }, *tmp, *buf_ptr;
	int count = 0;
	char key_name[MAX_STR] = {0, };
	int grab_mode = KEYGRAB_TYPE_SHARED;
	int key_type = KEY_ALL;

	while ((c = getchar()) != EOF) {
		if (c == '\n') break;
		if (count >= MAX_STR) break;

		buf[count] = c;
		count++;
	}

	count = 0;
	tmp = strtok_r(buf, " ", &buf_ptr);
	if (!tmp) return ECORE_CALLBACK_RENEW;

	if (!strncmp(buf, "q", MAX_STR) || !strncmp(buf, "quit", MAX_STR)) {
		ecore_main_loop_quit();
	}
	else if (!strncmp(buf, "help", MAX_STR)) {
		_usage();
	}
	else if (!strncmp(tmp, "grab", sizeof("grab"))) {
		while (tmp) {
			tmp = strtok_r(NULL, " ", &buf_ptr);
			if (tmp) {
				switch (count) {
					case 0:
						strncpy(key_name, tmp, MAX_STR - 1);
						break;
					case 1:
						grab_mode = atoi(tmp);
						break;
					default:
						break;
				}
			}
			count++;
		}
		if (strlen(key_name) <= 0) {
			printf("Please input valid arguments for key grab\n");
			_usage();
		}
		else
			_keygrab(key_name, grab_mode);
	}
	else if (!strncmp(tmp, "ungrab", sizeof("ungrab"))) {
		tmp = strtok_r(NULL, " ", &buf_ptr);
		if (tmp) {
			strncpy(key_name, tmp, MAX_STR - 1);
		}
		if (strlen(key_name) <= 0) {
			printf("Please input valid arguments for key ungrab\n");
			_usage();
		}
		else
			_keyungrab(key_name);
	}
	else if (!strncmp(tmp, "hide", sizeof("hide"))) {
		ecore_wl2_window_hide(client->win);
		ecore_wl2_window_commit(client->win, EINA_TRUE);
		printf("hide window\n");
	}
	else if (!strncmp(tmp, "show", sizeof("show"))) {
		ecore_wl2_window_show(client->win);
		ecore_wl2_window_activate(client->win);
		ecore_wl2_window_commit(client->win, EINA_TRUE);
		printf("show window\n");
	}
	else if (!strncmp(tmp, "update", sizeof("update"))) {
		tmp = strtok_r(NULL, " ", &buf_ptr);
		_update_window(client, tmp);
		printf("update window : %s\n", tmp);
	}
	else if (!strncmp(tmp, "inputgen", sizeof("inputgen"))) {
		tmp = strtok_r(NULL, " ", &buf_ptr);
		if (tmp) {
			if (!strncmp(tmp, "init", sizeof("init"))) {
				_inputgen_init(client);
			}
			else if (!strncmp(tmp, "deinit", sizeof("deinit"))) {
				_inputgen_deinit(client);
			}
			else if (!strncmp(tmp, "key", sizeof("key"))) {
				while (tmp) {
					tmp = strtok_r(NULL, " ", &buf_ptr);
					if (tmp) {
						switch (count) {
							case 0:
							strncpy(key_name, tmp, MAX_STR - 1);
							break;
						case 1:
							key_type = atoi(tmp);
							break;
						default:
							break;
						}
					}
					count++;
				}
				_key_gen(client, key_name, key_type);
			}
			else {
				printf("invalid arguments\n");
				_usage();
			}
		}
	}
	else {
		printf("Invalid arguments\n");
		_usage();
	}

	return ECORE_CALLBACK_RENEW;
}

static void
_event_handlers_init(app_data_t *client)
{
	Ecore_Event_Handler *h = NULL;

	_ecore_event_hdls = eina_array_new(10);
	h = ecore_event_handler_add(ECORE_WL2_EVENT_FOCUS_IN, _cb_focus_in, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_FOCUS_OUT, _cb_focus_out, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_SHOW, _cb_window_show, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_HIDE, _cb_window_hide, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_LOWER, _cb_window_lower, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_ACTIVATE, _cb_window_activate, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_DEACTIVATE, _cb_window_deactivate, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _cb_key_down, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _cb_key_up, client);
	eina_array_push(_ecore_event_hdls, h);
}

int main(int argc, char **argv)
{
	int x, y, w, h;
	app_data_t *client = NULL;
	const char *socket_name = NULL;
	Eina_Bool focus_skip = EINA_FALSE;

	setvbuf(stdout, NULL, _IONBF, 0);

	/* get skip focus */
	if (getenv("HEADLESS_SKIP_FOCUS"))
		focus_skip = EINA_TRUE;

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "wayland-0";

	client = (app_data_t *)calloc(1, sizeof(app_data_t));
	ERROR_CHECK(client, goto shutdown, "Failed to allocate memory for app_data_t");

	if (!ecore_wl2_init())
	{
		TRACE("Failed to initialize ecore_wl2");
		goto shutdown;
	}

	client->ewd = ecore_wl2_display_connect(socket_name);
	ERROR_CHECK(client->ewd, goto shutdown, "Failed to connect to wayland display %s", socket_name);

	client->wl_tbm_client = wayland_tbm_client_init(ecore_wl2_display_get(client->ewd));
	ERROR_CHECK(client->wl_tbm_client, goto shutdown, "Failed to init wayland_tbm_client");

	_event_handlers_init(client);

	/*Create Sample Window*/
	x = y = 0;
	w = h = 1;
	client->win = ecore_wl2_window_new(client->ewd, NULL, x, y, w, h);
	ecore_wl2_window_alpha_set(client->win, EINA_FALSE);
	ecore_wl2_window_show(client->win);
	ecore_wl2_window_focus_skip_set(client->win, focus_skip);
	ecore_wl2_window_activate(client->win);
	ecore_wl2_window_commit(client->win, EINA_TRUE);

	client->tbm_queue = wayland_tbm_client_create_surface_queue(client->wl_tbm_client,
												ecore_wl2_window_surface_get(client->win),
												2,
												100, 100,
												TBM_FORMAT_ABGR8888);
	ERROR_CHECK(client->tbm_queue, goto shutdown, "Failed to create tbm_surface_queue");

	_usage();
	ecore_main_fd_handler_add(STDIN_FILENO, ECORE_FD_READ, _stdin_cb, client, NULL, NULL);

	/*Start Loop*/
	ecore_main_loop_begin();

shutdown:
	if (client) {
		if (client->tbm_queue)
			tbm_surface_queue_destroy(client->tbm_queue);

		if (client->wl_tbm_client)
			wayland_tbm_client_deinit(client->wl_tbm_client);

		ecore_wl2_shutdown();
		free(client);
	}

	return EXIT_SUCCESS;
}

