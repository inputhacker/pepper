/*
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

#include "pepper-keyrouter.h"
#include "pepper-internal.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct resources_data resources_data_t;

struct pepper_keyrouter {
	struct wl_global *global;
	struct wl_display *display;
	pepper_compositor_t *compositor;

	pepper_list_t resources;

	keyrouter_t *keyrouter;
};

struct resources_data {
	struct wl_resource *resource;
	pepper_list_t link;
};

static void
_pepper_keyrouter_cb_keygrab_set(struct wl_client *client,
                                         struct wl_resource *resource,
                                         struct wl_resource *surface,
                                         uint32_t key,
                                         uint32_t mode)
{
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, mode, TIZEN_KEYROUTER_ERROR_NONE);
}

static void
_pepper_keyrouter_cb_keygrab_unset(struct wl_client *client,
                                           struct wl_resource *resource,
                                           struct wl_resource *surface,
                                           uint32_t key)
{
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, TIZEN_KEYROUTER_MODE_NONE, TIZEN_KEYROUTER_ERROR_NONE);
}

static void
_pepper_keyrouter_cb_get_keygrab_status(struct wl_client *client,
                                                struct wl_resource *resource,
                                                struct wl_resource *surface,
                                                uint32_t key)
{
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, TIZEN_KEYROUTER_MODE_NONE, TIZEN_KEYROUTER_ERROR_NO_PERMISSION);
}

static void
_pepper_keyrouter_cb_keygrab_set_list(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface,
                                              struct wl_array *grab_list)
{
	tizen_keyrouter_send_keygrab_notify_list(resource, surface, grab_list);
}

static void
_pepper_keyrouter_cb_keygrab_unset_list(struct wl_client *client,
                                                struct wl_resource *resource,
                                                struct wl_resource *surface,
                                                struct wl_array *ungrab_list)
{
	tizen_keyrouter_send_keygrab_notify_list(resource, surface, ungrab_list);
}

static void
_pepper_keyrouter_cb_get_keygrab_list(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface)
{
	tizen_keyrouter_send_getgrab_notify_list(resource, surface, NULL);
}

static void
_pepper_keyrouter_cb_set_register_none_key(struct wl_client *client,
                                                   struct wl_resource *resource,
                                                   struct wl_resource *surface,
                                                   uint32_t data)
{
	tizen_keyrouter_send_set_register_none_key_notify(resource, NULL, 0);
}

static void
_pepper_keyrouter_cb_get_keyregister_status(struct wl_client *client,
                                                    struct wl_resource *resource,
                                                    uint32_t key)
{
	tizen_keyrouter_send_keyregister_notify(resource, (int)PEPPER_FALSE);
}

static void
_pepper_keyrouter_cb_set_input_config(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface,
                                              uint32_t config_mode,
                                              uint32_t value)
{
	tizen_keyrouter_send_set_input_config_notify(resource, (int)PEPPER_FALSE);
}

static const struct tizen_keyrouter_interface _pepper_keyrouter_implementation = {
   _pepper_keyrouter_cb_keygrab_set,
   _pepper_keyrouter_cb_keygrab_unset,
   _pepper_keyrouter_cb_get_keygrab_status,
   _pepper_keyrouter_cb_keygrab_set_list,
   _pepper_keyrouter_cb_keygrab_unset_list,
   _pepper_keyrouter_cb_get_keygrab_list,
   _pepper_keyrouter_cb_set_register_none_key,
   _pepper_keyrouter_cb_get_keyregister_status,
   _pepper_keyrouter_cb_set_input_config
};

/* tizen_keyrouter global object destroy function */
static void
_pepper_keyrouter_cb_destory(struct wl_resource *resource)
{
	resources_data_t *rdata, *rtmp;
	pepper_list_t *list;
	struct wl_client *client;
	pepper_keyrouter_t *pepper_keyrouter;

	PEPPER_CHECK(resource, return, "Invalid keyrouter resource\n");
	client = wl_resource_get_client(resource);
	PEPPER_CHECK(client, return, "Invalid client\n");
	pepper_keyrouter = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_keyrouter, return, "Invalid pepper_keyrouter_t\n");

	list = &pepper_keyrouter->resources;

	if (pepper_list_empty(list))
		return;

	pepper_list_for_each_safe(rdata, rtmp, list, link) {
		if (rdata->resource== resource) {
			pepper_list_remove(&rdata->link);
			free(rdata);
		}
	}
}

/* tizen_keyrouter global object bind function */
static void
_pepper_keyrouter_cb_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
	resources_data_t *rdata;
	pepper_keyrouter_t *pepper_keyrouter;

	pepper_keyrouter = (pepper_keyrouter_t *)data;
	PEPPER_CHECK(client, return, "Invalid client\n");
	PEPPER_CHECK(pepper_keyrouter, return, "Invalid pepper_keyrouter_t\n");

	resource = wl_resource_create(client, &tizen_keyrouter_interface, MIN(version, 1), id);
	if (!resource) {
		PEPPER_ERROR("Failed to create resource ! (version :%d, id:%d)", version, id);
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &_pepper_keyrouter_implementation,
	                               pepper_keyrouter, _pepper_keyrouter_cb_destory);

	rdata = (resources_data_t *)calloc(1, sizeof(resources_data_t));
	rdata->resource = resource;
	pepper_list_init(&rdata->link);

	pepper_list_insert(&pepper_keyrouter->resources, &rdata->link);
}

PEPPER_API pepper_keyrouter_t *
pepper_keyrouter_create(pepper_compositor_t *compositor)
{
	struct wl_display *display = NULL;
	struct wl_global *global = NULL;
	pepper_keyrouter_t *pepper_keyrouter;

	PEPPER_CHECK(compositor, return PEPPER_FALSE, "Invalid compositor\n");

	display = pepper_compositor_get_display(compositor);
	PEPPER_CHECK(display, return PEPPER_FALSE, "Failed to get wl_display from compositor\n");

	pepper_keyrouter = (pepper_keyrouter_t *)calloc(1, sizeof(pepper_keyrouter_t));
	PEPPER_CHECK(pepper_keyrouter, return PEPPER_FALSE, "Failed to allocate memory for keyrouter\n");
	pepper_keyrouter->display = display;
	pepper_keyrouter->compositor = compositor;

	pepper_list_init(&pepper_keyrouter->resources);

	global = wl_global_create(display, &tizen_keyrouter_interface, 1, pepper_keyrouter, _pepper_keyrouter_cb_bind);
	PEPPER_CHECK(global, goto failed, "Failed to create wl_global for tizen_keyrouter\n");

	pepper_keyrouter->keyrouter = keyrouter_create();
	PEPPER_CHECK(pepper_keyrouter->keyrouter, goto failed, "Failed to create keyrouter\n");

	return pepper_keyrouter;

failed:
	if (pepper_keyrouter) {
		if (pepper_keyrouter->keyrouter) {
			keyrouter_destroy(pepper_keyrouter->keyrouter);
			pepper_keyrouter->keyrouter = NULL;
		}
		free(pepper_keyrouter);
	}

	return NULL;
}

PEPPER_API void
pepper_keyrouter_destroy(pepper_keyrouter_t *pepper_keyrouter)
{
	resources_data_t *rdata, *rtmp;

	PEPPER_CHECK(pepper_keyrouter, return, "Pepper keyrouter is not initialized\n");

	pepper_list_for_each_safe(rdata, rtmp, &pepper_keyrouter->resources, link) {
		wl_resource_destroy(rdata->resource);
		pepper_list_remove(&rdata->link);
		free(rdata);
	}

	if (pepper_keyrouter->keyrouter) {
		keyrouter_destroy(pepper_keyrouter->keyrouter);
		pepper_keyrouter->keyrouter = NULL;
	}

	if (pepper_keyrouter->global)
		wl_global_destroy(pepper_keyrouter->global);

	free(pepper_keyrouter);
	pepper_keyrouter = NULL;
}
