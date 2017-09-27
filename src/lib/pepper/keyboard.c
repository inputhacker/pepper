/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
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

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

#include "pepper-internal.h"

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_impl = {
	keyboard_release,
};

static void
update_modifiers(pepper_keyboard_t *keyboard)
{
	if ((keyboard->pending.mods_depressed != keyboard->mods_depressed) ||
		(keyboard->pending.mods_latched != keyboard->mods_latched) ||
		(keyboard->pending.mods_locked != keyboard->mods_locked) ||
		(keyboard->pending.group != keyboard->group)) {

		keyboard->mods_depressed = keyboard->pending.mods_depressed;
		keyboard->mods_latched = keyboard->pending.mods_latched;
		keyboard->mods_locked = keyboard->pending.mods_locked;
		keyboard->group = keyboard->pending.group;

		keyboard->grab->modifiers(keyboard,
								  keyboard->data,
								  keyboard->mods_depressed,
								  keyboard->mods_latched,
								  keyboard->mods_locked, keyboard->group);
	}
}

static void
update_keymap(pepper_keyboard_t *keyboard)
{
	struct wl_resource *resource;

	if (keyboard->keymap_fd >= 0)
		close(keyboard->keymap_fd);

	keyboard->keymap_format = keyboard->pending.keymap_format;
	keyboard->keymap_fd = keyboard->pending.keymap_fd;
	keyboard->keymap_len = keyboard->pending.keymap_len;
	keyboard->pending.keymap_fd = -1;

	if (keyboard->keymap_format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP) {
		int fd = open("/dev/null", O_RDONLY);

		wl_resource_for_each(resource, &keyboard->resource_list)
			wl_keyboard_send_keymap(resource,
									WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP, fd,
									0);

		close(fd);
	} else {
		wl_resource_for_each(resource, &keyboard->resource_list)
			wl_keyboard_send_keymap(resource, keyboard->keymap_format,
									keyboard->keymap_fd,
									keyboard->keymap_len);

	}

	keyboard->need_update_keymap = 0;
}

void
pepper_keyboard_handle_event(pepper_keyboard_t *keyboard, uint32_t id,
							 pepper_input_event_t *event)
{
	uint32_t *keys = keyboard->keys.data;
	unsigned int num_keys = keyboard->keys.size / sizeof(uint32_t);
	unsigned int i;

	if (id != PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY)
		return;

	/* Update the array of pressed keys. */
	for (i = 0; i < num_keys; i++) {
		if (keys[i] == event->key) {
			keys[i] = keys[--num_keys];
			break;
		}
	}

	keyboard->keys.size = num_keys * sizeof(uint32_t);

	if (event->state == PEPPER_KEY_STATE_PRESSED)
		*(uint32_t *) wl_array_add(&keyboard->keys, sizeof(uint32_t)) = event->key;

	if (keyboard->grab)
		keyboard->grab->key(keyboard, keyboard->data, event->time, event->key,
							event->state);

	if (keyboard->need_update_keymap && keyboard->keys.size == 0) {
		pepper_object_emit_event(&keyboard->base,
							PEPPER_EVENT_KEYBOARD_KEYMAP_UPDATE, keyboard);
		update_keymap(keyboard);
		update_modifiers(keyboard);
	}

	pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_KEYBOARD_KEY,
							 event);
}

static void
keyboard_handle_focus_destroy(pepper_event_listener_t *listener,
							  pepper_object_t *surface,
							  uint32_t id, void *info, void *data)
{
	pepper_keyboard_t *keyboard = data;
	pepper_keyboard_set_focus(keyboard, NULL);

	if (keyboard->grab)
		keyboard->grab->cancel(keyboard, keyboard->data);
}

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t * seat)
{
	pepper_keyboard_t *keyboard =
		(pepper_keyboard_t *) pepper_object_alloc(PEPPER_OBJECT_KEYBOARD,
												  sizeof(pepper_keyboard_t));

	PEPPER_CHECK(keyboard, return NULL, "pepper_object_alloc() failed.\n");

	keyboard->seat = seat;
	keyboard->keymap_fd = -1;
	keyboard->pending.keymap_fd = -1;

	wl_list_init(&keyboard->resource_list);

	wl_array_init(&keyboard->keys);

	return keyboard;
}

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard)
{
	if (keyboard->grab)
		keyboard->grab->cancel(keyboard, keyboard->data);

	if (keyboard->focus)
		pepper_event_listener_remove(keyboard->focus_destroy_listener);

	if (keyboard->keymap_fd >= 0)
		close(keyboard->keymap_fd);
	if (keyboard->pending.keymap_fd >= 0)
		close(keyboard->pending.keymap_fd);

	wl_array_release(&keyboard->keys);
	free(keyboard);
}

static void
unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

void
pepper_keyboard_bind_resource(struct wl_client *client,
							  struct wl_resource *resource, uint32_t id)
{
	pepper_seat_t *seat =
		(pepper_seat_t *) wl_resource_get_user_data(resource);
	pepper_keyboard_t *keyboard = seat->keyboard;
	struct wl_resource *res;

	if (!keyboard)
		return;

	res = wl_resource_create(client, &wl_keyboard_interface,
							 wl_resource_get_version(resource), id);
	if (!res) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&keyboard->resource_list, wl_resource_get_link(res));
	wl_resource_set_implementation(res, &keyboard_impl, keyboard,
								   unbind_resource);

	/* TODO: send repeat info */
	if ((keyboard->keymap_format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP)
		|| (keyboard->keymap_fd < 0) || (keyboard->keymap_len == 0)) {
		int fd = open("/dev/null", O_RDONLY);
		if (fd < 0)
			return;
		wl_keyboard_send_keymap(res, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP,
								fd, 0);
		close(fd);
	} else {
		wl_keyboard_send_keymap(res, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
								keyboard->keymap_fd, keyboard->keymap_len);
		wl_keyboard_send_modifiers(res,
								   wl_display_next_serial(keyboard->seat->compositor->display),
								   keyboard->mods_depressed,
								   keyboard->mods_latched,
								   keyboard->mods_locked, keyboard->group);
	}

	if (!keyboard->focus || !keyboard->focus->surface ||
		!keyboard->focus->surface->resource)
		return;

	if (wl_resource_get_client(keyboard->focus->surface->resource) == client) {
		wl_keyboard_send_enter(res, keyboard->focus_serial,
							   keyboard->focus->surface->resource,
							   &keyboard->keys);
	}
}

/**
 * Get the list of wl_resource of the given keyboard
 *
 * @param keyboard  keyboard object
 *
 * @return list of the keyboard resources
 */
PEPPER_API struct wl_list *
pepper_keyboard_get_resource_list(pepper_keyboard_t *keyboard)
{
	return &keyboard->resource_list;
}

/**
 * Get the compositor of the given keyboard
 *
 * @param keyboard  keyboard object
 *
 * @return compositor
 */
PEPPER_API pepper_compositor_t *
pepper_keyboard_get_compositor(pepper_keyboard_t *keyboard)
{
	return keyboard->seat->compositor;
}

/**
 * Get the seat of the given keyboard
 *
 * @param keyboard  keyboard object
 *
 * @return seat
 */
PEPPER_API pepper_seat_t *
pepper_keyboard_get_seat(pepper_keyboard_t *keyboard)
{
	return keyboard->seat;
}

/**
 * Set the focus view of the given keyboard
 *
 * @param keyboard  keyboard object
 * @param focus     focus view
 *
 * @see pepper_keyboard_send_enter()
 * @see pepper_keyboard_send_leave()
 * @see pepper_keyboard_get_focus()
 */
PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *focus)
{
	if (keyboard->focus == focus)
		return;

	if (keyboard->focus) {
		pepper_event_listener_remove(keyboard->focus_destroy_listener);
		pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_FOCUS_LEAVE,
								 keyboard->focus);
		pepper_object_emit_event(&keyboard->focus->base,
								 PEPPER_EVENT_FOCUS_LEAVE, keyboard);
	}

	keyboard->focus = focus;

	if (focus) {
		keyboard->focus_serial =
			wl_display_next_serial(keyboard->seat->compositor->display);

		keyboard->focus_destroy_listener =
			pepper_object_add_event_listener(&focus->base,
											 PEPPER_EVENT_OBJECT_DESTROY, 0,
											 keyboard_handle_focus_destroy,
											 keyboard);

		pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_FOCUS_ENTER,
								 focus);
		pepper_object_emit_event(&focus->base, PEPPER_EVENT_FOCUS_ENTER,
								 keyboard);
	}
}

/**
 * Get the focus view of the given keyboard
 *
 * @param keyboard   keyboard object
 *
 * @return focus view
 *
 * @see pepper_keyboard_set_focus()
 */
PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard)
{
	return keyboard->focus;
}

/**
 * Send wl_keyboard.leave event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the leave event
 */
PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard, pepper_view_t *view)
{
	struct wl_resource *resource;
	struct wl_client *client;
	uint32_t serial;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_leave(resource, serial, view->surface->resource);
	}
}

/**
 * Send wl_keyboard.enter event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the enter event
 */
PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard, pepper_view_t *view)
{
	struct wl_resource *resource;
	struct wl_client *client;
	uint32_t serial;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client) {
			wl_keyboard_send_enter(resource, serial, view->surface->resource,
								   &keyboard->keys);
		}
	}
}

/**
 * Send wl_keyboard.key event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the enter event
 * @param time      time in mili-second with undefined base
 * @param key       key code
 * @param state     state flag (ex. WL_KEYBOARD_KEY_STATE_PRESSED)
 */
PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, pepper_view_t *view,
						 uint32_t time, uint32_t key, uint32_t state)
{
	struct wl_resource *resource;
	struct wl_client *client;
	uint32_t serial;
	pepper_input_event_t event;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_key(resource, serial, time, key, state);
	}

	event.time = time;
	event.key = key;
	event.state = state;
	pepper_object_emit_event(&view->base, PEPPER_EVENT_KEYBOARD_KEY, &event);
}

/**
 * Send wl_keyboard.key event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the enter event
 * @param depressed (none)
 * @param latched   (none)
 * @param locked    (none)
 * @param group     (none)
 */
PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard,
							   pepper_view_t *view, uint32_t depressed,
							   uint32_t latched, uint32_t locked,
							   uint32_t group)
{
	struct wl_resource *resource;
	struct wl_client *client;
	uint32_t serial;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_modifiers(resource, serial, depressed, latched,
									   locked, group);
	}
}

/**
 * Install keyboard grab
 *
 * @param keyboard  keyboard object
 * @param grab      grab handler
 * @param data      user data to be passed to grab functions
 */
PEPPER_API void
pepper_keyboard_set_grab(pepper_keyboard_t *keyboard,
						 const pepper_keyboard_grab_t *grab, void *data)
{
	keyboard->grab = grab;
	keyboard->data = data;
}

/**
 * Get the current keyboard grab
 *
 * @param keyboard  keyboard object
 *
 * @return grab handler which is most recently installed
 *
 * @see pepper_keyboard_set_grab()
 * @see pepper_keyboard_get_grab_data()
 */
PEPPER_API const pepper_keyboard_grab_t *
pepper_keyboard_get_grab(pepper_keyboard_t *keyboard)
{
	return keyboard->grab;
}

/**
 * Get the current keyboard grab data
 *
 * @param keyboard  keyboard object
 *
 * @return grab data which is most recently installed
 *
 * @see pepper_keyboard_set_grab()
 * @see pepper_keyboard_get_grab()
 */
PEPPER_API void *
pepper_keyboard_get_grab_data(pepper_keyboard_t *keyboard)
{
	return keyboard->data;
}

/**
 * Set xkb keymap for the given keyboard
 *
 * @param keyboard  keyboard object
 * @param keymap    xkb keymap
 *
 * This function might send wl_pointer.keymap and wl_pointer.modifers events internally
 */
PEPPER_DEPRECATED void
pepper_keyboard_set_keymap(pepper_keyboard_t *keyboard,
						   void *keymap);

/**
 * Set xkb keymap information for the given keyboard
 *
 * @param keyboard  keyboard object
 * @param keymap_format    wayland keymap format
 * @param keymap_fd        keymap file descriptor
 * @param keymap_len       length of file
 *
 * This function might send wl_keyboard.keymap
 */
PEPPER_API void
pepper_keyboard_set_keymap_info(pepper_keyboard_t *keyboard,
								uint32_t keymap_format,
								int keymap_fd, uint32_t keymap_len)
{
	if (keyboard->pending.keymap_fd >= 0)
		close(keyboard->pending.keymap_fd);

	keyboard->pending.keymap_fd = dup(keymap_fd);
	keyboard->pending.keymap_len = keymap_len;
	keyboard->pending.keymap_format = keymap_format;

	if (keyboard->keys.size == 0) {
		pepper_object_emit_event(&keyboard->base,
							PEPPER_EVENT_KEYBOARD_KEYMAP_UPDATE, keyboard);
		update_keymap(keyboard);
	} else {
		keyboard->need_update_keymap = 1;
	}
}

/**
 * Set modifiers for the given keyboard
 *
 * @param keyboard     keyboard object
 * @param depressed
 * @param latched
 * @param locked
 * @param group
 *
 * This function might send wl_keyboard.modifiers
 */
PEPPER_API void
pepper_keyboard_set_modifiers(pepper_keyboard_t *keyboard,
							  uint32_t depressed, uint32_t latched,
							  uint32_t locked, uint32_t group)
{
	keyboard->pending.mods_depressed = depressed;
	keyboard->pending.mods_latched = latched;
	keyboard->pending.mods_locked = locked;
	keyboard->pending.group = group;

	if (!keyboard->need_update_keymap)
		update_modifiers(keyboard);
}

/**
 * Get current xkb_info for the given keyboard
 *
 * @param keyboard  keyboard object
 */
PEPPER_API void *
pepper_keyboard_get_xkb_info(pepper_keyboard_t *keyboard)
{
	return keyboard->xkb_info;
}

/**
 * Get pending xkb_info for the given keyboard
 *
 * @param keyboard  keyboard object
 */
PEPPER_API void *
pepper_keyboard_get_pending_xkb_info(pepper_keyboard_t *keyboard)
{
	return keyboard->pending.xkb_info;
}

/**
 * Set xkb_info for the given keyboard
 *
 * @param keyboard  keyboard object
 * @param xkb_info    xkb_info
 */
PEPPER_API void
pepper_keyboard_set_xkb_info(pepper_keyboard_t *keyboard,
								void *xkb_info)
{
	keyboard->xkb_info = xkb_info;
}

/**
 * Set pending xkb_info for the given keyboard
 *
 * @param keyboard  keyboard object
 * @param xkb_info    xkb_info
 */
PEPPER_API void
pepper_keyboard_set_pending_xkb_info(pepper_keyboard_t *keyboard,
								void *xkb_info)
{
	keyboard->pending.xkb_info = xkb_info;
}
