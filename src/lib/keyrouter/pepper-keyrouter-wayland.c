/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2017 Samsung Electronics co., Ltd. All Rights Reserved.
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

#include "pepper-keyrouter-wayland.h"
#include "pepper-internal.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct pepper_keyrouter_wl pepper_keyrouter_wl_t;
typedef struct resources_data resources_data_t;
typedef struct clients_data clients_data_t;
typedef struct grab_list_data grab_list_data_t;
typedef struct ungrab_list_data ungrab_list_data_t;

struct pepper_keyrouter_wl
{
	struct wl_global *global;
	struct wl_display *display;
	pepper_compositor_t *compositor;

	pepper_list_t resources;
	pepper_list_t clients;

	pepper_keyrouter_t *keyrouter;
};

struct resources_data
{
	struct wl_resource *resource;
	pepper_list_t link;
};

struct clients_data
{
	struct wl_client *client;
	pepper_list_t link;
};

struct grab_list_data
{
	int key;
	int mode;
	int err;
};

struct ungrab_list_data
{
	int key;
	int err;
};

pepper_keyrouter_wl_t *keyrouter_wl = NULL;

static void
_pepper_keyrouter_wl_key_send(pepper_seat_t *seat, struct wl_client *client,
                              unsigned int key, unsigned int state,
                              unsigned int time)
{
	struct wl_resource *resource;
	pepper_keyboard_t *keyboard_info;

	keyboard_info = pepper_seat_get_keyboard(seat);

	wl_resource_for_each(resource, pepper_keyboard_get_resource_list(keyboard_info)) {
		if (wl_resource_get_client(resource) == client)
		{
			wl_keyboard_send_key(resource, wl_display_get_serial(keyrouter_wl->display), time, key, state);
			PEPPER_TRACE("[%s] key : %d, state : %d, time : %d\n", __FUNCTION__, key, state, time);
		}
	}
}

PEPPER_API void
pepper_keyrouter_wl_key_process(void *data, unsigned int key, unsigned int state, unsigned int time)
{
	pepper_list_t delivery_list;
	pepper_list_t *seat_list;
	keyrouter_key_info_t *info;
	int count = 0;
	pepper_seat_t *seat = (pepper_seat_t *)data;

	pepper_list_init(&delivery_list);
	count = pepper_keyrouter_key_process(keyrouter_wl->keyrouter, key, state, &delivery_list);

	if (count > 0) {
		pepper_list_for_each(info, &delivery_list, link) {
			if (seat && pepper_object_get_type((pepper_object_t *)seat) == PEPPER_OBJECT_SEAT) {
				_pepper_keyrouter_wl_key_send(seat, (struct wl_client *)info->data, key, state, time);
			}
			else {
				seat_list = (pepper_list_t *)pepper_compositor_get_seat_list(keyrouter_wl->compositor);
				if (!pepper_list_empty(seat_list)) {
					pepper_list_for_each(seat, seat_list, link) {
						_pepper_keyrouter_wl_key_send(seat, (struct wl_client *)info->data, key, state, time);
					}
				}
			}
		}
	}
}

static void
_pepper_keyrouter_wl_remove_client_from_list(struct wl_client *client)
{
	int i;
	for (i = 0; i < PEPPER_KEYROUTER_MAX_KEYS; i++) {
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_EXCLUSIVE,
	                           i, (void *)client);
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_OR_EXCLUSIVE,
	                           i, (void *)client);
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_TOP_POSITION,
	                           i, (void *)client);
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_SHARED,
	                           i, (void *)client);
	}
}


static void
_pepper_keyrouter_wl_client_cb_destroy(struct wl_listener *l, void *data)
{
	struct wl_client *client = (struct wl_client *)data;
	clients_data_t *cdata, *tmp;

	_pepper_keyrouter_wl_remove_client_from_list(client);

	wl_list_remove(&l->link);
	free(l);
	l = NULL;

	pepper_list_for_each_safe(cdata, tmp, &keyrouter_wl->clients, link) {
		if (cdata->client == client) {
			pepper_list_remove(&cdata->link);
			free(cdata);
		}
	}
}

static void
_pepper_keyrouter_wl_client_destory_handler_add(struct wl_client *client)
{
	struct wl_listener *destroy_listener = NULL;
	clients_data_t *data = NULL;

	PEPPER_CHECK(client, return, "Invalid client\n");
	pepper_list_for_each(data, &keyrouter_wl->clients, link) {
		if (data->client == client) return;
	}

	data = NULL;

	destroy_listener = (struct wl_listener *)calloc(1, sizeof(struct wl_listener));
	PEPPER_CHECK(destroy_listener, return, "Failed to allocate memory for %p client destory listener\n", client);
	data = (clients_data_t *)calloc(1, sizeof(clients_data_t));
	PEPPER_CHECK(data, return, "Failed to allocate memory for client data\n");
	pepper_list_init(&data->link);

	destroy_listener->notify = _pepper_keyrouter_wl_client_cb_destroy;
	wl_client_add_destroy_listener(client, destroy_listener);
	pepper_list_insert(&keyrouter_wl->clients, &data->link);
}

static void
_pepper_keyrouter_wl_cb_keygrab_set(struct wl_client *client,
                                         struct wl_resource *resource,
                                         struct wl_resource *surface,
                                         uint32_t key,
                                         uint32_t mode)
{
	int res = TIZEN_KEYROUTER_ERROR_NONE;
	res = pepper_keyrouter_grab_key(keyrouter_wl->keyrouter, mode, key, (void *)client);
	if (res == TIZEN_KEYROUTER_ERROR_NONE) {
		_pepper_keyrouter_wl_client_destory_handler_add(client);
	}
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, mode, res);
}

static void
_pepper_keyrouter_wl_cb_keygrab_unset(struct wl_client *client,
                                           struct wl_resource *resource,
                                           struct wl_resource *surface,
                                           uint32_t key)
{
	pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_EXCLUSIVE,
	                           key, (void *)client);
	pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_OR_EXCLUSIVE,
	                           key, (void *)client);
	pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_TOP_POSITION,
	                           key, (void *)client);
	pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
	                           KEYROUTER_GRAB_TYPE_SHARED,
	                           key, (void *)client);

	tizen_keyrouter_send_keygrab_notify(resource, surface, key, TIZEN_KEYROUTER_MODE_NONE, TIZEN_KEYROUTER_ERROR_NONE);
}

static void
_pepper_keyrouter_wl_cb_get_keygrab_status(struct wl_client *client,
                                                struct wl_resource *resource,
                                                struct wl_resource *surface,
                                                uint32_t key)
{
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, TIZEN_KEYROUTER_MODE_NONE, TIZEN_KEYROUTER_ERROR_NO_PERMISSION);
}

static int
_pepper_keyrouter_wl_array_length(const struct wl_array *array)
{
	int *data = NULL;
	int count = 0;

	wl_array_for_each(data, array) {
		count++;
	}

	return count;
}

static void
_pepper_keyrouter_wl_cb_keygrab_set_list(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface,
                                              struct wl_array *grab_list)
{
	struct wl_array *return_list = NULL;
	grab_list_data_t *grab_data = NULL;
	int res = TIZEN_KEYROUTER_ERROR_NONE;

	PEPPER_CHECK(grab_list, goto notify, "Please send valid grab_list\n");
	PEPPER_CHECK(((_pepper_keyrouter_wl_array_length(grab_list) %3) == 0), goto notify,
	             "Invalid keycode and grab mode pair. Check arguments in a list\n");

	wl_array_for_each(grab_data, grab_list) {
		res = pepper_keyrouter_grab_key(keyrouter_wl->keyrouter, grab_data->mode, grab_data->key, (void *)client);
		grab_data->err = res;
	}

	return_list = grab_list;
notify:
	tizen_keyrouter_send_keygrab_notify_list(resource, surface, return_list);
}

static void
_pepper_keyrouter_wl_cb_keygrab_unset_list(struct wl_client *client,
                                                struct wl_resource *resource,
                                                struct wl_resource *surface,
                                                struct wl_array *ungrab_list)
{
	struct wl_array *return_list = NULL;
	ungrab_list_data_t *ungrab_data = NULL;

	PEPPER_CHECK(((_pepper_keyrouter_wl_array_length(ungrab_list) %3) == 0), goto notify,
	             "Invalid keycode and grab mode pair. Check arguments in a list\n");

	wl_array_for_each(ungrab_data, ungrab_list) {
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
		                            KEYROUTER_GRAB_TYPE_EXCLUSIVE,
		                            ungrab_data->key, (void *)client);
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
		                            KEYROUTER_GRAB_TYPE_OR_EXCLUSIVE,
		                            ungrab_data->key, (void *)client);
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
		                            KEYROUTER_GRAB_TYPE_TOP_POSITION,
		                            ungrab_data->key, (void *)client);
		pepper_keyrouter_ungrab_key(keyrouter_wl->keyrouter,
		                            KEYROUTER_GRAB_TYPE_SHARED,
		                            ungrab_data->key, (void *)client);

		ungrab_data->err = TIZEN_KEYROUTER_ERROR_NONE;;
	}

	return_list = ungrab_list;
notify:
	tizen_keyrouter_send_keygrab_notify_list(resource, surface, return_list);
}

static void
_pepper_keyrouter_wl_cb_get_keygrab_list(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface)
{
	tizen_keyrouter_send_getgrab_notify_list(resource, surface, NULL);
}

static void
_pepper_keyrouter_wl_cb_set_register_none_key(struct wl_client *client,
                                                   struct wl_resource *resource,
                                                   struct wl_resource *surface,
                                                   uint32_t data)
{
	tizen_keyrouter_send_set_register_none_key_notify(resource, NULL, 0);
}

static void
_pepper_keyrouter_wl_cb_get_keyregister_status(struct wl_client *client,
                                                    struct wl_resource *resource,
                                                    uint32_t key)
{
	tizen_keyrouter_send_keyregister_notify(resource, (int)PEPPER_FALSE);
}

static void
_pepper_keyrouter_wl_cb_set_input_config(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface,
                                              uint32_t config_mode,
                                              uint32_t value)
{
	tizen_keyrouter_send_set_input_config_notify(resource, (int)PEPPER_FALSE);
}

static const struct tizen_keyrouter_interface _pepper_keyrouter_wl_implementation = {
   _pepper_keyrouter_wl_cb_keygrab_set,
   _pepper_keyrouter_wl_cb_keygrab_unset,
   _pepper_keyrouter_wl_cb_get_keygrab_status,
   _pepper_keyrouter_wl_cb_keygrab_set_list,
   _pepper_keyrouter_wl_cb_keygrab_unset_list,
   _pepper_keyrouter_wl_cb_get_keygrab_list,
   _pepper_keyrouter_wl_cb_set_register_none_key,
   _pepper_keyrouter_wl_cb_get_keyregister_status,
   _pepper_keyrouter_wl_cb_set_input_config
};

/* tizen_keyrouter global object destroy function */
static void
_pepper_keyrouter_wl_cb_destory(struct wl_resource *resource)
{
	resources_data_t *data, *tmp;
	pepper_list_t *list;

	list = &keyrouter_wl->resources;

	if (pepper_list_empty(list))
		return;

	pepper_list_for_each_safe(data, tmp, list, link) {
		if (data->resource== resource) {
			pepper_list_remove(&data->link);
			free(data);
		}
	}
}

/* tizen_keyrouter global object bind function */
static void
_pepper_keyrouter_wl_cb_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
	resources_data_t *rdata;

	PEPPER_CHECK(client, return, "Invalid client\n");
	resource = wl_resource_create(client, &tizen_keyrouter_interface, MIN(version, 1), id);
	if (!resource) {
		PEPPER_ERROR("Failed to create resource ! (version :%d, id:%d)", version, id);
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &_pepper_keyrouter_wl_implementation,
	                               NULL, _pepper_keyrouter_wl_cb_destory);

	rdata = (resources_data_t *)calloc(1, sizeof(resources_data_t));
	rdata->resource = resource;
	pepper_list_init(&rdata->link);

	pepper_list_insert(&keyrouter_wl->resources, &rdata->link);
}

PEPPER_API void
pepper_keyrouter_wl_event_handler(pepper_event_listener_t *listener,
                               pepper_object_t *object,
                               uint32_t id, void *info, void *data)
{
	pepper_input_event_t *event;

	PEPPER_CHECK((id == PEPPER_EVENT_KEYBOARD_KEY),
	             return, "%d event is not handled by keyrouter\n", id);
	PEPPER_CHECK(info, return, "Invalid event\n");

	event = (pepper_input_event_t *)info;
	pepper_keyrouter_wl_key_process(data, event->key, event->state, event->time);
}

PEPPER_API void
pepper_keyrouter_wl_grab_print(void)
{
	PEPPER_CHECK(keyrouter_wl, return, "Wayland Keyrouter is not initialized\n");
	PEPPER_CHECK(keyrouter_wl->keyrouter, return, "Keyrouter is not initialized\n");

	pepper_keyrouter_print_list(keyrouter_wl->keyrouter);
}

PEPPER_API pepper_bool_t
pepper_keyrouter_wl_init(pepper_compositor_t *compositor)
{
	struct wl_display *display = NULL;
	struct wl_global *global = NULL;

	PEPPER_CHECK(!keyrouter_wl, return PEPPER_FALSE, "Already initialized pepper wayland keyrouter\n");
	PEPPER_CHECK(compositor, return PEPPER_FALSE, "Invalid compositor\n");

	display = pepper_compositor_get_display(compositor);
	PEPPER_CHECK(display, return PEPPER_FALSE, "Failed to get wl_display from compositor\n");

	keyrouter_wl = (pepper_keyrouter_wl_t *)calloc(1, sizeof(pepper_keyrouter_wl_t));
	PEPPER_CHECK(keyrouter_wl, return PEPPER_FALSE, "Failed to allocate memory for keyrouter_wl\n");
	keyrouter_wl->display = display;
	keyrouter_wl->compositor = compositor;

	pepper_list_init(&keyrouter_wl->resources);
	pepper_list_init(&keyrouter_wl->clients);

	global = wl_global_create(display, &tizen_keyrouter_interface, 1, NULL, _pepper_keyrouter_wl_cb_bind);
	PEPPER_CHECK(global, goto failed, "Failed to create wl_global for tizen_keyrouter\n");

	keyrouter_wl->keyrouter = pepper_keyrouter_create();
	PEPPER_CHECK(keyrouter_wl->keyrouter, goto failed, "Failed to create keyrouter\n");

	return PEPPER_TRUE;

failed:
	if (keyrouter_wl) {
		if (keyrouter_wl->keyrouter) {
			pepper_keyrouter_destroy(keyrouter_wl->keyrouter);
			keyrouter_wl->keyrouter = NULL;
		}
		free(keyrouter_wl);
	}

	return PEPPER_FALSE;
}

PEPPER_API void
pepper_keyrouter_wl_deinit(void)
{
	resources_data_t *rdata, *rtmp;
	clients_data_t *cdata, *ctmp;
	struct wl_listener *destroy_listener;

	PEPPER_CHECK(keyrouter_wl, return, "Pepper keyrouter is not initialized\n");

	pepper_list_for_each_safe(cdata, ctmp, &keyrouter_wl->clients, link) {
		destroy_listener = wl_client_get_destroy_listener(cdata->client,
		                       _pepper_keyrouter_wl_client_cb_destroy);
		if (destroy_listener) {
			wl_list_remove(&destroy_listener->link);
			free(destroy_listener);
		}
		pepper_list_remove(&cdata->link);
		free(cdata);
	}

	pepper_list_for_each_safe(rdata, rtmp, &keyrouter_wl->resources, link) {
		wl_resource_destroy(rdata->resource);
		pepper_list_remove(&rdata->link);
		free(rdata);
	}

	if (keyrouter_wl->keyrouter) {
		pepper_keyrouter_destroy(keyrouter_wl->keyrouter);
		keyrouter_wl->keyrouter = NULL;
	}

	if (keyrouter_wl->global)
		wl_global_destroy(keyrouter_wl->global);

	free(keyrouter_wl);
	keyrouter_wl = NULL;
}
