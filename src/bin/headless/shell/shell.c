/*
* Copyright © 2018 Samsung Electronics co., Ltd. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pepper.h>
#include <xdg-shell-unstable-v6-server-protocol.h>

typedef struct {
	pepper_compositor_t *compositor;
	struct wl_global *zxdg_shell;

	pepper_seat_t *seat;
	pepper_event_listener_t *seat_add_listener;
	pepper_event_listener_t  *seat_remove_listener;
}headless_shell_t;

typedef struct {
	pepper_view_t *view;
	struct wl_resource *zxdg_shell_surface;
	uint32_t last_ack_configure;
}headless_shell_surface_t;

const static int KEY_SHELL = 0;

static void
zxdg_toplevel_cb_resource_destroy(struct wl_resource *resource)
{
}

static void
zxdg_toplevel_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
zxdg_toplevel_cb_parent_set(struct wl_client *client,
			    struct wl_resource *resource,
			    struct wl_resource *res_parent)
{
}

static void
zxdg_toplevel_cb_title_set(struct wl_client *client,
			   struct wl_resource *resource,
			   const char *title)
{
}

static void
zxdg_toplevel_cb_app_id_set(struct wl_client *client,
			    struct wl_resource *resource,
			    const char *app_id)
{
}

static void
zxdg_toplevel_cb_win_menu_show(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *res_seat,
			       uint32_t serial,
			       int32_t x,
			       int32_t y)
{
}

static void
zxdg_toplevel_cb_move(struct wl_client *client,
		      struct wl_resource *resource,
		      struct wl_resource *res_seat,
		      uint32_t serial)
{
}

static void
zxdg_toplevel_cb_resize(struct wl_client *client,
			struct wl_resource *resource,
			struct wl_resource *res_seat,
			uint32_t serial,
			uint32_t edges)
{
}

static void
zxdg_toplevel_cb_max_size_set(struct wl_client *client,
			      struct wl_resource *resource,
			      int32_t w,
			      int32_t h)
{
}

static void
zxdg_toplevel_cb_min_size_set(struct wl_client *client,
			      struct wl_resource *resource,
			      int32_t w,
			      int32_t h)
{
}

static void
zxdg_toplevel_cb_maximized_set(struct wl_client *client, struct wl_resource *resource)
{
}

static void
zxdg_toplevel_cb_maximized_unset(struct wl_client *client, struct wl_resource *resource)
{
}

static void
zxdg_toplevel_cb_fullscreen_set(struct wl_client *client,
				struct wl_resource *resource,
				struct wl_resource *res_output)
{
}

static void
zxdg_toplevel_cb_fullscreen_unset(struct wl_client *client, struct wl_resource *resource)
{
}

static void
zxdg_toplevel_cb_minimized_set(struct wl_client *client, struct wl_resource *resource)
{
}

static const struct zxdg_toplevel_v6_interface zxdg_toplevel_interface =
{
	zxdg_toplevel_cb_destroy,
	zxdg_toplevel_cb_parent_set,
	zxdg_toplevel_cb_title_set,
	zxdg_toplevel_cb_app_id_set,
	zxdg_toplevel_cb_win_menu_show,
	zxdg_toplevel_cb_move,
	zxdg_toplevel_cb_resize,
	zxdg_toplevel_cb_max_size_set,
	zxdg_toplevel_cb_min_size_set,
	zxdg_toplevel_cb_maximized_set,
	zxdg_toplevel_cb_maximized_unset,
	zxdg_toplevel_cb_fullscreen_set,
	zxdg_toplevel_cb_fullscreen_unset,
	zxdg_toplevel_cb_minimized_set
};

static void
zxdg_popup_cb_resource_destroy(struct wl_resource *resource)
{
}

static void
zxdg_popup_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
zxdg_popup_cb_grab(struct wl_client *client,
		   struct wl_resource *resource,
		   struct wl_resource *res_seat,
		   uint32_t serial)
{
}

static const struct zxdg_popup_v6_interface zxdg_popup_interface =
{
	zxdg_popup_cb_destroy,
	zxdg_popup_cb_grab
};

static void
zxdg_surface_cb_resource_destroy(struct wl_resource *resource)
{
	headless_shell_surface_t *hs_surface;

	hs_surface = wl_resource_get_user_data(resource);
	PEPPER_CHECK(hs_surface, return, "fail to get hs_surface\n");

	hs_surface->zxdg_shell_surface = NULL;
	pepper_view_destroy(hs_surface->view);

	free(hs_surface);
}

static void
zxdg_surface_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
zxdg_surface_cb_toplevel_get(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	struct wl_resource *new_res;
	new_res = wl_resource_create(client, &zxdg_toplevel_v6_interface, 1, id);
	if (!new_res) {
		PEPPER_ERROR("fail to create zxdg_toplevel");
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(new_res,
			&zxdg_toplevel_interface,
			NULL,
			zxdg_toplevel_cb_resource_destroy);
}

static void
zxdg_surface_cb_popup_get(struct wl_client *client,
			  struct wl_resource *resource,
		uint32_t id,
		struct wl_resource *res_parent,
		struct wl_resource *res_pos)
{
	struct wl_resource *new_res;
	new_res = wl_resource_create(client, &zxdg_popup_v6_interface, 1, id);
	if (!new_res) {
		PEPPER_ERROR("fail to create popup");
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(new_res,
	                          &zxdg_popup_interface,
	                          NULL,
	                          zxdg_popup_cb_resource_destroy);
}

static void
zxdg_surface_cb_win_geometry_set(struct wl_client *client,
                                   struct wl_resource *resource,
                                   int32_t x,
                                   int32_t y,
                                   int32_t w,
                                   int32_t h)
{
}

static void
zxdg_surface_cb_configure_ack(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	headless_shell_surface_t *hs_surface;

	hs_surface = wl_resource_get_user_data(resource);
	PEPPER_CHECK(hs_surface, return, "fail to get headless_shell_surface\n");

	hs_surface->last_ack_configure = serial;
}

static const struct zxdg_surface_v6_interface zxdg_surface_interface =
{
	zxdg_surface_cb_destroy,
	zxdg_surface_cb_toplevel_get,
	zxdg_surface_cb_popup_get,
	zxdg_surface_cb_win_geometry_set,
	zxdg_surface_cb_configure_ack
};

static void
zxdg_positioner_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
zxdg_positioner_cb_size_set(struct wl_client *client,
			    struct wl_resource *resource,
			    int32_t w, int32_t h)
{
}

static void
zxdg_positioner_cb_anchor_rect_set(struct wl_client *client,
				   struct wl_resource *resource,
				   int32_t x, int32_t y, int32_t w, int32_t h)
{
}

static void
zxdg_positioner_cb_anchor_set(struct wl_client *client,
			      struct wl_resource *resource,
			      enum zxdg_positioner_v6_anchor anchor)
{
}

static void
zxdg_positioner_cb_gravity_set(struct wl_client *client,
			       struct wl_resource *resource,
			       enum zxdg_positioner_v6_gravity gravity)
{
}

static void
zxdg_positioner_cb_constraint_adjustment_set(struct wl_client *client,
					     struct wl_resource *resource,
					     enum zxdg_positioner_v6_constraint_adjustment constraint_adjustment)
{
}

static void
zxdg_positioner_cb_offset_set(struct wl_client *client,
			      struct wl_resource *resource,
			      int32_t x, int32_t y)
{
}

static const struct zxdg_positioner_v6_interface zxdg_positioner_interface =
{
	zxdg_positioner_cb_destroy,
	zxdg_positioner_cb_size_set,
	zxdg_positioner_cb_anchor_rect_set,
	zxdg_positioner_cb_anchor_set,
	zxdg_positioner_cb_gravity_set,
	zxdg_positioner_cb_constraint_adjustment_set,
	zxdg_positioner_cb_offset_set,
};

static void
zxdg_shell_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
	PEPPER_TRACE("Destroy zxdg_shell\n");

	wl_resource_destroy(resource);
}

static void
zxdg_shell_cb_positioner_create(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	struct wl_resource *new_res;

	PEPPER_TRACE("Create zxdg_positoiner\n");

	new_res = wl_resource_create(client, &zxdg_positioner_v6_interface, 1, id);
	if (!new_res)
	{
		PEPPER_ERROR("fail to create zxdg_positioner\n");
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(new_res, &zxdg_positioner_interface, NULL, NULL);
}

static void
zxdg_shell_cb_surface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *wsurface)
{
	headless_shell_t *hs;
	headless_shell_surface_t *hs_surface;
	pepper_surface_t *psurface;

	hs = wl_resource_get_user_data(resource);
	if (!hs) {
		PEPPER_ERROR("fail to get headless_shell\n");
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to get headless shell");
		return;
	}

	hs_surface = (headless_shell_surface_t *)calloc(sizeof(headless_shell_surface_t), 1);
	if (!hs_surface) {
		PEPPER_ERROR("fail to alloc for headless_shell_surface_t\n");
		wl_resource_post_no_memory(resource);
		return;
	}

	hs_surface->zxdg_shell_surface = wl_resource_create(client, &zxdg_surface_v6_interface, 1, id);
	if (!hs_surface->zxdg_shell_surface) {
		PEPPER_ERROR("fail to create the zxdg_surface\n");
		wl_resource_post_no_memory(resource);
		goto error;
	}

	wl_resource_set_implementation(hs_surface->zxdg_shell_surface,
			&zxdg_surface_interface,
			hs_surface,
			zxdg_surface_cb_resource_destroy);


	psurface = wl_resource_get_user_data(wsurface);
	PEPPER_CHECK(psurface, goto error, "faile to get pepper_surface\n");

	hs_surface->view = pepper_compositor_add_view(hs->compositor);
	if (!hs_surface->view) {
		PEPPER_ERROR("fail to create the pepper_view\n");
		wl_resource_post_no_memory(resource);
		goto error;
	}

	if (!pepper_view_set_surface(hs_surface->view, psurface)) {
		PEPPER_ERROR("fail to set surface\n");
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
				"Assign set psurface to pview");
		goto error;
	}

	if (!pepper_surface_set_role(psurface, "xdg_surface")) {
		PEPPER_ERROR("fail to set role\n");
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
				"Assign \"xdg_surface\" to wl_surface failed\n");
		goto error;
	}

	PEPPER_TRACE("create zxdg_surface:%p, pview:%p, psurface:%p\n",
			hs_surface->zxdg_shell_surface,
			hs_surface->view,
			psurface);

	/* temporary focus view set */
	if (!hs->seat)
		return;

	pepper_keyboard_t *keyboard = pepper_seat_get_keyboard(hs->seat);
	PEPPER_CHECK(keyboard, return, "[%s] pepper_keyboard is null !", __FUNCTION__);

	pepper_view_t *focus = pepper_keyboard_get_focus(keyboard);

	if (!focus || focus != hs_surface->view) {
		if (focus) pepper_keyboard_send_leave(keyboard, focus);
		pepper_keyboard_set_focus(keyboard, hs_surface->view);
		pepper_keyboard_send_enter(keyboard, hs_surface->view);
	}

	/* temporary top view set */
	pepper_view_stack_top(hs_surface->view, PEPPER_FALSE);

	return;
error:
	if (hs_surface) {
		if (hs_surface->view)
			pepper_view_destroy(hs_surface->view);

		if (hs_surface->zxdg_shell_surface)
			wl_resource_destroy(hs_surface->zxdg_shell_surface);
		else
			free(hs_surface);
	}
}

static void
zxdg_shell_cb_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{

}

static const struct zxdg_shell_v6_interface zxdg_shell_interface =
{
	zxdg_shell_cb_destroy,
	zxdg_shell_cb_positioner_create,
	zxdg_shell_cb_surface_get,
	zxdg_shell_cb_pong
};

static void
zxdg_shell_cb_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
	headless_shell_t *hs = (headless_shell_t *)data;

	PEPPER_TRACE("Bind zxdg_shell\n");

	/* Create resource for zxdg_shell_v6 */
	resource = wl_resource_create(client,
			&zxdg_shell_v6_interface,
			version,
			id);
	PEPPER_CHECK(resource, goto err_shell, "fail to create the zxdg_shell_v6\n");

	wl_resource_set_implementation(resource, &zxdg_shell_interface, hs, NULL);

	return;
err_shell:
	wl_resource_destroy(resource);
	wl_client_post_no_memory(client);
}

static pepper_bool_t
zxdg_init(headless_shell_t *shell)
{
	struct wl_display *display;

	display = pepper_compositor_get_display(shell->compositor);

	shell->zxdg_shell = wl_global_create(display, &zxdg_shell_v6_interface, 1, shell, zxdg_shell_cb_bind);
	PEPPER_CHECK(shell->zxdg_shell, return PEPPER_FALSE, "fail to create zxdg_shell\n");

	return PEPPER_TRUE;
}

void
zxdg_deinit(headless_shell_t *shell)
{
	if (shell->zxdg_shell)
		wl_global_destroy(shell->zxdg_shell);
}

static void
_seat_add_callback(pepper_event_listener_t    *listener,
				  pepper_object_t            *object,
				  uint32_t                    id,
				  void                       *info,
				  void                       *data)
{
	headless_shell_t *shell = (headless_shell_t *)data;
	shell->seat = (pepper_seat_t *)info;

	if (shell->seat)
		PEPPER_TRACE("[%s] seat added (name:%s)\n", __FUNCTION__, pepper_seat_get_name(shell->seat));
	else
		PEPPER_TRACE("[%s] seat is NULL.\n", __FUNCTION__);
}

static void
_seat_remove_callback(pepper_event_listener_t    *listener,
					 pepper_object_t            *object,
					 uint32_t                    id,
					 void                       *info,
					 void                       *data)
{
	headless_shell_t *shell = (headless_shell_t *)data;

	if (shell->seat)
		PEPPER_TRACE("[%s] seat removed (name:%s)\n", __FUNCTION__, pepper_seat_get_name(shell->seat));
	else
		PEPPER_TRACE("[%s] seat is NULL.\n", __FUNCTION__);

	shell->seat = NULL;
}

static void
init_listeners(headless_shell_t *shell)
{
	shell->seat_add_listener = pepper_object_add_event_listener((pepper_object_t *)shell->compositor,
								PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
								0, _seat_add_callback, shell);

	shell->seat_remove_listener = pepper_object_add_event_listener((pepper_object_t *)shell->compositor,
								PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
								0, _seat_remove_callback, shell);
}

static void
deinit_listeners(headless_shell_t *shell)
{
	pepper_event_listener_remove(shell->seat_add_listener);
	pepper_event_listener_remove(shell->seat_remove_listener);
}

static void
headless_shell_deinit(void *data)
{
	headless_shell_t *shell = (headless_shell_t*)data;
	if (!shell) return;

	deinit_listeners(shell);
	zxdg_deinit(shell);

	pepper_object_set_user_data((pepper_object_t *)shell->compositor, &KEY_SHELL, NULL, NULL);
	free(shell);
}

pepper_bool_t
headless_shell_init(pepper_compositor_t *compositor)
{
	headless_shell_t *shell;

	shell = (headless_shell_t*)calloc(sizeof(headless_shell_t), 1);
	PEPPER_CHECK(shell, goto error, "fail to alloc for shell\n");
	shell->compositor = compositor;

	init_listeners(shell);
	PEPPER_CHECK(zxdg_init(shell), goto error, "zxdg_init() failed\n");

	pepper_object_set_user_data((pepper_object_t *)compositor, &KEY_SHELL, NULL, headless_shell_deinit);

	return PEPPER_TRUE;
error:
	headless_shell_deinit(shell);
	return PEPPER_FALSE;
}
