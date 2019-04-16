#include <stdio.h>
#include <stdlib.h>
#include <Ecore_Wl2.h>
#include <Ecore_Input.h>

#define DISPLAY_NAME "headless-0"

#define DEBUG
#ifdef DEBUG
#define TRACE(fmt, ...)	\
    do { \
        printf("[headless-client : %s] "fmt, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)
#else
#define TRACE(fmt, ...)
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
struct app_data
{
	Ecore_Wl2_Display *ewd;
	Ecore_Wl2_Window *win;
};

static Eina_Array *_ecore_event_hdls;

static Eina_Bool
_cb_focus_in(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Focus_In *ev = (Ecore_Wl2_Event_Focus_In *)event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_focus_out(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Focus_Out *ev = (Ecore_Wl2_Event_Focus_Out *)event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_show(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Show *ev = (Ecore_Wl2_Event_Window_Show *)event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_lower(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Lower *ev = (Ecore_Wl2_Event_Window_Lower *)event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_activate(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Activate *ev = (Ecore_Wl2_Event_Window_Activate *)event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_window_deactivate(void *data, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Wl2_Event_Window_Deactivate *ev = (Ecore_Wl2_Event_Window_Deactivate *)event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_key_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Event_Key *ev = event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_key_up(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
	app_data_t *client =  (app_data_t *)data;
	Ecore_Event_Key *ev = event;

	TRACE("");

	/* TODO */

	return ECORE_CALLBACK_PASS_ON;
}

static void _event_handlers_init(app_data_t *client)
{
	Ecore_Event_Handler *h = NULL;

	_ecore_event_hdls = eina_array_new(10);
	h = ecore_event_handler_add(ECORE_WL2_EVENT_FOCUS_IN, _cb_focus_in, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_FOCUS_OUT, _cb_focus_out, client);
	eina_array_push(_ecore_event_hdls, h);

	h = ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_SHOW, _cb_window_show, client);
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

	client = (app_data_t *)calloc(1, sizeof(app_data_t));
	ERROR_CHECK(client, goto shutdown, "Failed to allocate memory for app_data_t");

	if (!ecore_wl2_init())
	{
		TRACE("Failed to initialize ecore_wl2");
		goto shutdown;
	}

	client->ewd = ecore_wl2_display_connect(DISPLAY_NAME);
	ERROR_CHECK(client->ewd, goto shutdown, "Failed to connect to wayland display %s", DISPLAY_NAME);

	_event_handlers_init(client);

	x = y = 0;
	w = h = 1;
	client->win = ecore_wl2_window_new(client->ewd, NULL, x, y, w, h);

	/* TODO */

	return EXIT_SUCCESS;

shutdown:
	return EXIT_FAILURE;
}

