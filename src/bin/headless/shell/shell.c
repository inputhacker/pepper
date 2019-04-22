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
#include <tizen-extension-server-protocol.h>

#include "headless_server.h"

typedef struct {
	pepper_compositor_t *compositor;
	struct wl_global *zxdg_shell;
	struct wl_global *tizen_policy;

	pepper_event_listener_t *surface_add_listener;
	pepper_event_listener_t *surface_remove_listener;
}headless_shell_t;

typedef struct {
	pepper_surface_t *surface;
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
	headless_shell_surface_t *hs_surface = NULL;
	pepper_surface_t *psurface;

	hs = wl_resource_get_user_data(resource);
	if (!hs) {
		PEPPER_ERROR("fail to get headless_shell\n");
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to get headless shell");
		return;
	}

	psurface = wl_resource_get_user_data(wsurface);
	PEPPER_CHECK(psurface, goto error, "faile to get pepper_surface\n");

	hs_surface = (headless_shell_surface_t *)pepper_object_get_user_data((pepper_object_t *)psurface, wsurface);
	if (!hs_surface) {
		PEPPER_ERROR("fail to get headless_shell_surface_t: %p key:%p\n", psurface, wsurface);
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

	return;
error:
	if (hs_surface) {
		if (hs_surface->view)
			pepper_view_destroy(hs_surface->view);

		if (hs_surface->zxdg_shell_surface)
			wl_resource_destroy(hs_surface->zxdg_shell_surface);
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
tizen_visibility_cb_destroy(struct wl_client *client, struct wl_resource *res_tzvis)
{
	wl_resource_destroy(res_tzvis);
}

static const struct tizen_visibility_interface tizen_visibility =
{
	tizen_visibility_cb_destroy
};

static void
tizen_visibility_cb_vis_destroy(struct wl_resource *res_tzvis)
{
}

static void
tizen_position_cb_destroy(struct wl_client *client, struct wl_resource *res_tzpos)
{
   wl_resource_destroy(res_tzpos);
}

static void
tizen_position_cb_set(struct wl_client *client, struct wl_resource *res_tzpos, int32_t x, int32_t y)
{
}

static const struct tizen_position_interface tizen_position =
{
   tizen_position_cb_destroy,
   tizen_position_cb_set,
};

static void
tizen_position_cb_pos_destroy(struct wl_resource *res_tzpos)
{
}

static void
tizen_policy_cb_vis_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surf)
{
	struct wl_resource *new_res;

	new_res = wl_resource_create(client, &tizen_visibility_interface, 5, id);
	if (!new_res) {
		PEPPER_ERROR("fail to create tizen_visibility");
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(new_res,
			&tizen_visibility,
			NULL,
			tizen_visibility_cb_vis_destroy);
}

static void
tizen_policy_cb_pos_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surf)
{
	struct wl_resource *new_res;

	new_res = wl_resource_create(client, &tizen_position_interface, 1, id);
	if (!new_res) {
		PEPPER_ERROR("fail to create tizen_visibility");
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(new_res,
			&tizen_position,
			NULL,
			tizen_position_cb_pos_destroy);
}

static void
tizen_policy_cb_activate(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
	pepper_surface_t *psurface;
	headless_shell_surface_t *hs_surface;

	psurface = wl_resource_get_user_data(surf);
	PEPPER_CHECK(psurface, return, "fail to get pepper_surface_t\n");

	hs_surface = pepper_object_get_user_data((pepper_object_t *)psurface, surf);
	PEPPER_CHECK(hs_surface, return, "fail to get headless_shell_surface\n");

	/* FIXME: set a view of the given surface as the focus/top view */
	void *input = headless_input_get();
	headless_input_set_focus_view(input, hs_surface->view);
	headless_input_set_top_view(input, hs_surface->view);
}

static void
tizen_policy_cb_activate_below_by_res_id(struct wl_client *client, struct wl_resource *resource,  uint32_t res_id, uint32_t below_res_id)
{
}

static void
tizen_policy_cb_raise(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_lower(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_lower_by_res_id(struct wl_client *client, struct wl_resource *resource,  uint32_t res_id)
{
}

static void
tizen_policy_cb_focus_skip_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_focus_skip_unset(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_role_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, const char *role)
{
}

static void
tizen_policy_cb_type_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, uint32_t type)
{
}

static void
tizen_policy_cb_conformant_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_conformant_unset(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_conformant_get(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
   tizen_policy_send_conformant(resource, surf, 0);
}

static void
tizen_policy_cb_notilv_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, int32_t lv)
{
}

static void
tizen_policy_cb_transient_for_set(struct wl_client *client, struct wl_resource *resource, uint32_t child_id, uint32_t parent_id)
{
}

static void
tizen_policy_cb_transient_for_unset(struct wl_client *client, struct wl_resource *resource, uint32_t child_id)
{
   tizen_policy_send_transient_for_done(resource, child_id);
}

static void
tizen_policy_cb_win_scrmode_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, uint32_t mode)
{
   tizen_policy_send_window_screen_mode_done
     (resource, surf, mode, TIZEN_POLICY_ERROR_STATE_NONE);
}

static void
tizen_policy_cb_subsurf_place_below_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *subsurf)
{
}

static void
tizen_policy_cb_subsurf_stand_alone_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *subsurf)
{
}

static void
tizen_policy_cb_subsurface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, uint32_t parent_id)
{
	wl_client_post_implementation_error(client, "Headless server not support tizen_subsurface\n");
}

static void
tizen_policy_cb_opaque_state_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface, int32_t state)
{
}

static void
tizen_policy_cb_iconify(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_uniconify(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_aux_hint_add(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, int32_t id, const char *name, const char *value)
{
}

static void
tizen_policy_cb_aux_hint_change(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, int32_t id, const char *value)
{
}

static void
tizen_policy_cb_aux_hint_del(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, int32_t id)
{
}

static void
tizen_policy_cb_supported_aux_hints_get(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_background_state_set(struct wl_client *client, struct wl_resource *resource, uint32_t pid)
{
}

static void
tizen_policy_cb_background_state_unset(struct wl_client *client, struct wl_resource *resource, uint32_t pid)
{
}

static void
tizen_policy_cb_floating_mode_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_floating_mode_unset(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf)
{
}

static void
tizen_policy_cb_stack_mode_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surf, uint32_t mode)
{
}

static void
tizen_policy_cb_activate_above_by_res_id(struct wl_client *client, struct wl_resource *resource,  uint32_t res_id, uint32_t above_res_id)
{
}

static void
tizen_policy_cb_subsurf_watcher_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface)
{
	wl_client_post_implementation_error(client, "Headless server not support tizen_subsurface\n");
}

static void
tizen_policy_cb_parent_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *child, struct wl_resource *parent)
{
}

static void
tizen_policy_cb_ack_conformant_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface, uint32_t serial)
{
}

static void
tizen_policy_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
   wl_resource_destroy(resource);
}

static void
tizen_policy_cb_has_video(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface, uint32_t has)
{
}

static const struct tizen_policy_interface tizen_policy_iface =
{
   tizen_policy_cb_vis_get,
   tizen_policy_cb_pos_get,
   tizen_policy_cb_activate,
   tizen_policy_cb_activate_below_by_res_id,
   tizen_policy_cb_raise,
   tizen_policy_cb_lower,
   tizen_policy_cb_lower_by_res_id,
   tizen_policy_cb_focus_skip_set,
   tizen_policy_cb_focus_skip_unset,
   tizen_policy_cb_role_set,
   tizen_policy_cb_type_set,
   tizen_policy_cb_conformant_set,
   tizen_policy_cb_conformant_unset,
   tizen_policy_cb_conformant_get,
   tizen_policy_cb_notilv_set,
   tizen_policy_cb_transient_for_set,
   tizen_policy_cb_transient_for_unset,
   tizen_policy_cb_win_scrmode_set,
   tizen_policy_cb_subsurf_place_below_parent,
   tizen_policy_cb_subsurf_stand_alone_set,
   tizen_policy_cb_subsurface_get,
   tizen_policy_cb_opaque_state_set,
   tizen_policy_cb_iconify,
   tizen_policy_cb_uniconify,
   tizen_policy_cb_aux_hint_add,
   tizen_policy_cb_aux_hint_change,
   tizen_policy_cb_aux_hint_del,
   tizen_policy_cb_supported_aux_hints_get,
   tizen_policy_cb_background_state_set,
   tizen_policy_cb_background_state_unset,
   tizen_policy_cb_floating_mode_set,
   tizen_policy_cb_floating_mode_unset,
   tizen_policy_cb_stack_mode_set,
   tizen_policy_cb_activate_above_by_res_id,
   tizen_policy_cb_subsurf_watcher_get,
   tizen_policy_cb_parent_set,
   tizen_policy_cb_ack_conformant_region,
   tizen_policy_cb_destroy,
   tizen_policy_cb_has_video,
};

static void
tizen_policy_cb_unbind(struct wl_resource *resource)
{
}

static void
tizen_policy_cb_bind(struct wl_client *client, void *data, uint32_t ver, uint32_t id)
{
	headless_shell_t *shell = (headless_shell_t *)data;
	struct wl_resource *resource;

	PEPPER_CHECK(shell, goto err, "data is NULL\n");

	resource = wl_resource_create(client,
									&tizen_policy_interface,
									ver,
									id);
	PEPPER_CHECK(resource, goto err, "fail to create tizen_policy\n");

	wl_resource_set_implementation(resource,
									&tizen_policy_iface,
									shell,
									tizen_policy_cb_unbind);
	return;

err:
	wl_client_post_no_memory(client);
}

static pepper_bool_t
tizen_policy_init(headless_shell_t *shell)
{
	struct wl_display *display;

	display = pepper_compositor_get_display(shell->compositor);

	shell->tizen_policy = wl_global_create(display, &tizen_policy_interface, 7, shell, tizen_policy_cb_bind);
	PEPPER_CHECK(shell->tizen_policy, return PEPPER_FALSE, "faile to create tizen_policy\n");

	return PEPPER_TRUE;
}

void
tizen_policy_deinit(headless_shell_t *shell)
{
	if (shell->zxdg_shell)
		wl_global_destroy(shell->tizen_policy);
}

static void
headless_shell_cb_surface_free(void *data)
{
	headless_shell_surface_t *surface = (headless_shell_surface_t *)data;

	free(surface);
}

static void
headless_shell_cb_surface_add(pepper_event_listener_t *listener,
										pepper_object_t *object,
										uint32_t id, void *info, void *data)
{
	headless_shell_surface_t *hs_surface;
	pepper_surface_t *surface = (pepper_surface_t *)info;

	hs_surface = (headless_shell_surface_t*)calloc(sizeof(headless_shell_surface_t), 1);
	PEPPER_CHECK(hs_surface, return, "fail to alloc for headless_shell_surface\n");

	hs_surface->surface = (pepper_surface_t *)surface;
	pepper_object_set_user_data((pepper_object_t *)surface,
								pepper_surface_get_resource(surface),
								hs_surface,
								headless_shell_cb_surface_free);
	PEPPER_TRACE("Add headless_shell:%p to pepper_surface:%p\n", hs_surface, object);
}

static void
headless_shell_cb_surface_remove(pepper_event_listener_t *listener,
										pepper_object_t *object,
										uint32_t id, void *info, void *data)
{
	headless_shell_surface_t *hs_surface;

	hs_surface = (headless_shell_surface_t*)calloc(sizeof(headless_shell_surface_t), 1);
	PEPPER_CHECK(hs_surface, return, "fail to alloc for headless_shell_surface\n");

	hs_surface->surface = (pepper_surface_t *)info;
}

static void
headless_shell_init_listeners(headless_shell_t *shell)
{
	shell->surface_add_listener = pepper_object_add_event_listener((pepper_object_t *)shell->compositor,
																	PEPPER_EVENT_COMPOSITOR_SURFACE_ADD,
																	0, headless_shell_cb_surface_add, shell);

	shell->surface_remove_listener = pepper_object_add_event_listener((pepper_object_t *)shell->compositor,
																		PEPPER_EVENT_COMPOSITOR_SURFACE_REMOVE,
																		0, headless_shell_cb_surface_remove, shell);
}

static void
headless_shell_deinit_listeners(headless_shell_t *shell)
{
	pepper_event_listener_remove(shell->surface_add_listener);
	pepper_event_listener_remove(shell->surface_remove_listener);
}

static void
headless_shell_deinit(void *data)
{
	headless_shell_t *shell = (headless_shell_t*)data;
	if (!shell) return;

	headless_shell_deinit_listeners(shell);
	zxdg_deinit(shell);
	tizen_policy_deinit(shell);

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

	headless_shell_init_listeners(shell);
	PEPPER_CHECK(zxdg_init(shell), goto error, "zxdg_init() failed\n");
	PEPPER_CHECK(tizen_policy_init(shell), goto error, "tizen_policy_init() failed\n");

	pepper_object_set_user_data((pepper_object_t *)compositor, &KEY_SHELL, NULL, headless_shell_deinit);

	return PEPPER_TRUE;
error:
	headless_shell_deinit(shell);
	return PEPPER_FALSE;
}