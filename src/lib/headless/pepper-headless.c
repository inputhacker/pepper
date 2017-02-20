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

#include "headless-internal.h"

static void
_headless_destroy(struct wl_client *client,
			struct wl_resource *resource)
{
	/**
	 * destroy - destroy the tizen headless object
	 *
	 * Destroy the pepper headless object
	 */

	 wl_resource_destroy(resource);
}

static void
_headless_create_buffer(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t id,
			      uint32_t buffer_type)
{
	/**
	 * create_buffer - (none)
	 * @id: buffer to create
	 * @buffer_type: buffer type
	 */

	pepper_headless_t *headless = NULL;
	pepper_headless_buffer_t *headless_buffer = NULL;
	//pepper_list_t *list = NULL;

	headless = (pepper_headless_t *)wl_resource_get_user_data(resource);
	PEPPER_CHECK(headless, goto error, "Failed to get headless data from resource...\n");

	headless_buffer = (pepper_headless_buffer_t *)calloc(1, sizeof(pepper_headless_buffer_t));
	PEPPER_CHECK(headless_buffer, goto error, "Failed to allocate memory for pepper headless buffer...\n");

	headless_buffer->type = buffer_type;
	headless_buffer->buffer = (struct wl_resource *)wl_resource_create(client, &wl_buffer_interface, 1, id);
	PEPPER_CHECK(headless_buffer->buffer, goto error, "Failed to create a resource for a wl_buffer...\n");

	pepper_list_init(&headless_buffer->link);
	pepper_list_insert(&headless->buffers, &headless_buffer->link);

	return;

error:
	if (headless_buffer->buffer)
		wl_resource_destroy(headless_buffer->buffer);

	if (headless_buffer)
		free(headless_buffer);

	return;
}

static void
_headless_set_buffer_data_array(struct wl_client *client,
				      struct wl_resource *resource,
				      struct wl_resource *buffer,
				      struct wl_array *data)
{
	/**
	 * set_buffer_data_array - (none)
	 * @buffer: (none)
	 * @data: (none)
	 */

	//TODO : make association between the given buffer and the given data
}

static void
_headless_display_buffer(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *buffer)
{
	/**
	 * display_buffer - (none)
	 * @buffer: (none)
	 */

	//TODO : get the data from the given buffer and call display buffer callback of output backend
}

static const struct tizen_headless_interface headless_interface = {
	_headless_destroy,
	_headless_create_buffer,
	_headless_set_buffer_data_array,
	_headless_display_buffer,
};

static void
_cb_client_destroy(struct wl_listener *l, void *data)
{
	//struct wl_client *client = (struct wl_client *)data;

	wl_list_remove(&l->link);

	free(l);
	l = NULL;

	//TODO : Do something with given client if there are ...
}

static void
_client_destroy_handler_add(pepper_headless_t *headless, struct wl_client *client)
{
	struct wl_listener *destroy_listener = NULL;

	if (!client)
		return;

	destroy_listener = wl_client_get_destroy_listener(client, _cb_client_destroy);
	if (destroy_listener) return;

	destroy_listener = (struct wl_listener *)calloc(1, sizeof(struct wl_listener));
	PEPPER_CHECK(destroy_listener, goto error, "Failed to allocate memory for destroy listner for client...\n");

	destroy_listener->notify = _cb_client_destroy;
	wl_client_add_destroy_listener(client, destroy_listener);

	return;

error:
	if (destroy_listener)
		free(destroy_listener);

}

static void
_tizen_headless_cb_destroy(struct wl_resource *resource)
{
	pepper_headless_t *headless = NULL;
	pepper_headless_resource_t *res = NULL;
	pepper_headless_resource_t *tres = NULL;

	headless = (pepper_headless_t *)wl_resource_get_user_data(resource);
	PEPPER_CHECK(headless, return, "User data from resource is NULL...\n");

	if (pepper_list_empty(&headless->resources))
		return;

	pepper_list_for_each_safe(res, tres, &headless->resources , link)
	{
		if (resource == res->resource)
		{
			pepper_list_remove(&res->link);
			free(res);
			res = NULL;
		}
	}
}

static void
_tizen_headless_cb_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	pepper_headless_t *headless = (pepper_headless_t *)data;
	pepper_headless_resource_t *headless_resource = NULL;

	headless_resource = (pepper_headless_resource_t *)calloc(1, sizeof(pepper_headless_resource_t));
	PEPPER_CHECK(headless_resource, goto error, "Failed to allocate memory for headless resource...\n");

	headless_resource->resource = wl_resource_create(client, &tizen_headless_interface, 1, id);
	PEPPER_CHECK(headless_resource->resource, goto error, "Failed to create a resource fo headless interface...\n");

	wl_resource_set_implementation(headless_resource->resource, &headless_interface, headless, _tizen_headless_cb_destroy);

	pepper_list_init(&headless_resource->link);
	pepper_list_insert(&headless->resources, &headless_resource->link);

	return;

error:
	if (headless_resource->resource)
		wl_resource_destroy(headless_resource->resource);

	if (headless_resource)
		free(headless_resource);

	return;
}

PEPPER_API pepper_headless_t *
pepper_headless_create(pepper_compositor_t *compositor)
{
	pepper_headless_t *headless = NULL;
	struct wl_display *display = NULL;

	PEPPER_CHECK(compositor, goto error, "Invalid argument (compositor == NULL)...\n");

	headless = (pepper_headless_t *)calloc(1, sizeof(pepper_headless_t));
	PEPPER_CHECK(headless, goto error, "Failed to allocate memory for pepper_headless...\n");

	pepper_list_init(&headless->resources);
	pepper_list_init(&headless->buffers);

	headless->compositor = compositor;
	headless->global = wl_global_create(display, &tizen_headless_interface, 1, headless, _tizen_headless_cb_bind);
	PEPPER_CHECK(headless->global, goto error, "Failed to create global for tizen headless interface...\n");

	return headless;

error:
	if (headless)
		pepper_headless_destroy(headless);

	return NULL;
}

PEPPER_API void
pepper_headless_destroy(pepper_headless_t *headless)
{
	struct wl_client *client = NULL;
	pepper_headless_resource_t *res = NULL;
	pepper_headless_resource_t *tres = NULL;

	pepper_headless_buffer_t *buf = NULL;
	pepper_headless_buffer_t *tbuf = NULL;
	struct wl_listener *destroy_listener = NULL;

	if (!headless)
		return;

	//Clean-up buffer and the owning client
	if (!pepper_list_empty(&headless->buffers))
	{
		pepper_list_for_each_safe(buf, tbuf, &headless->buffers, link)
		{
			if (buf->buffer)
				wl_resource_destroy(buf->buffer);

			pepper_list_remove(&buf->link);
			free(buf);
			buf = NULL;
		}
	}

	//Clean-up resource and the owning client of it
	if (!pepper_list_empty(&headless->resources))
	{
		pepper_list_for_each_safe(res, tres, &headless->resources , link)
		{
			//Remove client destroy listener(s)
			client = wl_resource_get_client(res->resource);

			if (client)
			{
				destroy_listener = wl_client_get_destroy_listener(client, _cb_client_destroy);

				if (destroy_listener)
				{
					wl_list_remove(&destroy_listener->link);

					free(destroy_listener);
					destroy_listener = NULL;
				}
			}

			//TODO : Remove something assocated 'res' if there are ...

			//Destroy pepper_headless object and remove the resource from list
			if (res->resource)
				wl_resource_destroy(res->resource);

			pepper_list_remove(&res->link);
			free(res);
			res = NULL;
		}
	}

	if (headless->global)
		wl_global_destroy(headless->global);

	free(headless);
	headless = NULL;
}
