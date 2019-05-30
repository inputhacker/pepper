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
#include <tizen-extension-server-protocol.h>
#ifdef HAVE_CYNARA
#include <cynara-session.h>
#include <cynara-client.h>
#include <cynara-creds-socket.h>
#include <sys/smack.h>
#include <stdio.h>
#include <stdarg.h>
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct resources_data resources_data_t;
typedef struct clients_data clients_data_t;
typedef struct grab_list_data grab_list_data_t;
typedef struct ungrab_list_data ungrab_list_data_t;

struct pepper_keyrouter {
	struct wl_global *global;
	struct wl_display *display;
	pepper_compositor_t *compositor;
	pepper_seat_t *seat;
	pepper_keyboard_t *keyboard;

	pepper_list_t resources;
	pepper_list_t grabbed_clients;

	keyrouter_t *keyrouter;

	pepper_view_t *focus_view;
	pepper_view_t *top_view;
#ifdef HAVE_CYNARA
	cynara *p_cynara;
#endif
};

struct resources_data {
	struct wl_resource *resource;
	pepper_list_t link;
};

struct clients_data {
	struct wl_client *client;
	pepper_list_t link;
};

struct grab_list_data {
	int key;
	int mode;
	int err;
};

struct ungrab_list_data {
	int key;
	int err;
};

#ifdef HAVE_CYNARA
static void
_pepper_keyrouter_util_cynara_log(int err, const char *fmt, ...)
{
#define CYNARA_BUFSIZE 128
	char buf[CYNARA_BUFSIZE] = "\0", tmp[CYNARA_BUFSIZE + CYNARA_BUFSIZE] = "\0";
	va_list args;
	int ret;

	if (fmt) {
		va_start(args, fmt);
		vsnprintf(tmp, CYNARA_BUFSIZE + CYNARA_BUFSIZE, fmt, args);
		va_end(args);
	}

	ret = cynara_strerror(err, buf, CYNARA_BUFSIZE);
	if (ret != CYNARA_API_SUCCESS) {
		PEPPER_ERROR("Failed to cynara_strerror: %d (error log about %s: %d)\n", ret, tmp, err);
		return;
	}

	PEPPER_ERROR("%s is failed: %s\n", tmp, buf);
}
#endif

static pepper_bool_t
_pepper_keyrouter_util_cynara_init(pepper_keyrouter_t *pepper_keyrouter)
{
#ifdef HAVE_CYNARA
	int ret;
	if (pepper_keyrouter->p_cynara) return PEPPER_TRUE;

	ret = cynara_initialize(&pepper_keyrouter->p_cynara, NULL);
	if (CYNARA_API_SUCCESS != ret) {
		_pepper_keyrouter_util_cynara_log(ret, "cynara_initialize");
		return PEPPER_FALSE;
	}
#endif
	return PEPPER_TRUE;
}

static void
_pepper_keyrouter_util_cynara_deinit(pepper_keyrouter_t *pepper_keyrouter)
{
#ifdef HAVE_CYNARA
	if (pepper_keyrouter->p_cynara) cynara_finish(pepper_keyrouter->p_cynara);
#else
	;
#endif
}

static pepper_bool_t
_pepper_keyrouter_util_do_privilege_check(pepper_keyrouter_t *pepper_keyrouter, struct wl_client *client, uint32_t mode, uint32_t keycode)
{
	pepper_bool_t res = PEPPER_TRUE;
#ifdef HAVE_CYNARA
	int ret, retry_cnt = 0, len = 0;
	char *clientSmack = NULL, *client_session = NULL, uid2[16]={0, };
	clients_data_t *cdata;
	static pepper_bool_t retried = PEPPER_FALSE;
	pid_t pid = 0;
	uid_t uid = 0;
	gid_t gid = 0;

	res = PEPPER_FALSE;

	/* Top position grab is always allowed. This mode do not need privilege.*/
	if (mode == TIZEN_KEYROUTER_MODE_TOPMOST) return PEPPER_TRUE;
	if (!client) return PEPPER_FALSE;

	/* If initialize cynara is failed, allow keygrabs regardless of the previlege permition. */
	if (pepper_keyrouter->p_cynara == NULL) {
		if (retried == PEPPER_FALSE) {
			retried = PEPPER_TRUE;
			for(retry_cnt = 0; retry_cnt < 5; retry_cnt++) {
				PEPPER_TRACE("Retry cynara initialize: %d\n", retry_cnt + 1);

				ret = cynara_initialize(&pepper_keyrouter->p_cynara, NULL);
				if (CYNARA_API_SUCCESS != ret) {
					_pepper_keyrouter_util_cynara_log(ret, "cynara_initialize retry..");
					pepper_keyrouter->p_cynara = NULL;
				} else {
					PEPPER_TRACE("Success cynara initialize to try %d times\n", retry_cnt + 1);
					break;
				}
			}
		}
		if (!pepper_keyrouter->p_cynara) return PEPPER_TRUE;
	}

	pepper_list_for_each(cdata, &pepper_keyrouter->grabbed_clients, link) {
		if (cdata->client == client) return PEPPER_TRUE;
	}

	wl_client_get_credentials(client, &pid, &uid, &gid);

	len = smack_new_label_from_process((int)pid, &clientSmack);
	if (len <= 0) goto finish;

	snprintf(uid2, 15, "%d", (int)uid);
	client_session = cynara_session_from_pid(pid);

	ret = cynara_check(pepper_keyrouter->p_cynara, clientSmack, client_session, uid2, "http://tizen.org/privilege/keygrab");
	if (CYNARA_API_ACCESS_ALLOWED == ret) {
		res = PEPPER_TRUE;
		PEPPER_TRACE("Success to check cynara, clientSmack: %s client_session: %s, uid2: %s\n", clientSmack, client_session, uid2);
	} else {
		//PEPPER_TRACE("Fail to check cynara, error : %d (pid : %d)", ret, pid);
		_pepper_keyrouter_util_cynara_log(ret, "clientsmack: %s, pid: %d", clientSmack, pid);
	}

finish:
	if (client_session) free(client_session);
	if (clientSmack) free(clientSmack);
#endif
	return res;
}

static struct wl_client *
_pepper_keyrouter_get_client_from_view(pepper_view_t *view)
{
	pepper_surface_t *surface = pepper_view_get_surface(view);
	PEPPER_CHECK(surface, return NULL, "No surfce available for the given view.\n");

	struct wl_resource *resource = pepper_surface_get_resource(surface);

	if (resource)
		return wl_resource_get_client(resource);

	return NULL;
}

static int
_pepper_keyrouter_get_pid(struct wl_client *client)
{
	pid_t pid = 0;
	uid_t uid = 0;
	gid_t gid = 0;

	wl_client_get_credentials(client, &pid, &uid, &gid);

	return pid;
}

PEPPER_API void
pepper_keyrouter_set_seat(pepper_keyrouter_t *pepper_keyrouter, pepper_seat_t *seat)
{
	PEPPER_CHECK(pepper_keyrouter, return, "Invalid pepper_keyrouter_t\n");
	pepper_keyrouter->seat = seat;
}

PEPPER_API void
pepper_keyrouter_set_keyboard(pepper_keyrouter_t * pepper_keyrouter, pepper_keyboard_t *keyboard)
{
	PEPPER_CHECK(pepper_keyrouter, return, "Invalid pepper_keyrouter_t\n");
	pepper_keyrouter->keyboard = keyboard;
}

PEPPER_API void
pepper_keyrouter_set_focus_view(pepper_keyrouter_t *pk, pepper_view_t *focus_view)
{
	struct wl_client *client = NULL;
	PEPPER_CHECK(pk, return, "pepper keyrouter is invalid.\n");
	PEPPER_CHECK(pk->keyboard, return, "No keyboard is available for pepper keyrouter.\n");

	pk->focus_view = focus_view;

	if (focus_view)
		client = _pepper_keyrouter_get_client_from_view(focus_view);

	keyrouter_set_focus_client(pk->keyrouter, client);
}

PEPPER_API void
pepper_keyrouter_set_top_view(pepper_keyrouter_t *pk, pepper_view_t *top_view)
{
	struct wl_client *client = NULL;
	PEPPER_CHECK(pk, return, "pepper keyrouter is invalid.\n");

	pk->top_view = top_view;

	if (top_view)
		client = _pepper_keyrouter_get_client_from_view(top_view);

	keyrouter_set_top_client(pk->keyrouter, client);
}

PEPPER_API void
pepper_keyrouter_debug_keygrab_status_print(pepper_keyrouter_t *pepper_keyrouter)
{
	pepper_list_t *list;
	int i;
	keyrouter_key_info_t *grab_data;

	PEPPER_CHECK(pepper_keyrouter, return, "pepper_keyrouter is invalid.\n");
	PEPPER_CHECK(pepper_keyrouter->keyrouter, return, "keyrouter is invalid.\n");

	for (i = 0; i < KEYROUTER_MAX_KEYS; i++) {
		if (keyrouter_is_grabbed_key(pepper_keyrouter->keyrouter, i)) {
			PEPPER_TRACE("\t[ Keycode: %d ]\n", i);
			list = keyrouter_grabbed_list_get(pepper_keyrouter->keyrouter, TIZEN_KEYROUTER_MODE_EXCLUSIVE, i);
			if (list && !pepper_list_empty(list)) {
				PEPPER_TRACE("\t  == Exclusive Grab ==\n");
				pepper_list_for_each(grab_data, list, link) {
					PEPPER_TRACE("\t    client: %p (pid: %d)\n", grab_data->data, _pepper_keyrouter_get_pid(grab_data->data));
				}
			}

			list = keyrouter_grabbed_list_get(pepper_keyrouter->keyrouter, TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE, i);
			if (list && !pepper_list_empty(list)) {
				PEPPER_TRACE("\t  == OR_Exclusive Grab ==\n");
				pepper_list_for_each(grab_data, list, link) {
					PEPPER_TRACE("\t    client: %p (pid: %d)\n", grab_data->data, _pepper_keyrouter_get_pid(grab_data->data));
				}
			}

			list = keyrouter_grabbed_list_get(pepper_keyrouter->keyrouter, TIZEN_KEYROUTER_MODE_TOPMOST, i);
			if (list && !pepper_list_empty(list)) {
				PEPPER_TRACE("\t  == Top Position Grab ==\n");
				pepper_list_for_each(grab_data, list, link) {
					PEPPER_TRACE("\t    client: %p (pid: %d)\n", grab_data->data, _pepper_keyrouter_get_pid(grab_data->data));
				}
			}

			list = keyrouter_grabbed_list_get(pepper_keyrouter->keyrouter, TIZEN_KEYROUTER_MODE_SHARED, i);
			if (list && !pepper_list_empty(list)) {
				PEPPER_TRACE("\t  == Shared Grab ==\n");
				pepper_list_for_each(grab_data, list, link) {
					PEPPER_TRACE("\t    client: %p (pid: %d)\n", grab_data->data, _pepper_keyrouter_get_pid(grab_data->data));
				}
			}
		}
	}
}

static void
_pepper_keyrouter_key_send(pepper_keyrouter_t *pepper_keyrouter,
                              pepper_seat_t *seat, struct wl_client *client,
                              unsigned int key, unsigned int state,
                              unsigned int time)
{
	struct wl_resource *resource;
	pepper_keyboard_t *keyboard_info;

	keyboard_info = pepper_seat_get_keyboard(seat);

	wl_resource_for_each(resource, pepper_keyboard_get_resource_list(keyboard_info)) {
		if (wl_resource_get_client(resource) == client)
		{
			wl_keyboard_send_key(resource, wl_display_get_serial(pepper_keyrouter->display), time, key, state);
			PEPPER_TRACE("[%s] key : %d, state : %d, time : %lu\n", __FUNCTION__, key, state, time);
		}
	}
}

PEPPER_API void
pepper_keyrouter_key_process(pepper_keyrouter_t *pepper_keyrouter,
                                unsigned int key, unsigned int state, unsigned int time)
{
	pepper_list_t delivery_list;
	pepper_list_t *seat_list;
	keyrouter_key_info_t *info;
	int count = 0;
	pepper_seat_t *seat;

	pepper_list_init(&delivery_list);

	/* Keygrab list is maintained by keycode + 8 which is used in xkb system */
	count = keyrouter_key_process(pepper_keyrouter->keyrouter, key + 8, state, &delivery_list);

	if (count > 0) {
		pepper_list_for_each(info, &delivery_list, link) {
			if (pepper_keyrouter->seat && pepper_object_get_type((pepper_object_t *)pepper_keyrouter->seat) == PEPPER_OBJECT_SEAT) {
				_pepper_keyrouter_key_send(pepper_keyrouter, pepper_keyrouter->seat, (struct wl_client *)info->data, key, state, time);
			}
			else if (pepper_keyrouter->keyboard && pepper_object_get_type((pepper_object_t *)pepper_keyrouter->keyboard) == PEPPER_OBJECT_KEYBOARD) {
				_pepper_keyrouter_key_send(pepper_keyrouter, pepper_keyboard_get_seat(pepper_keyrouter->keyboard), (struct wl_client *)info->data, key, state, time);
			}
			else {
				seat_list = (pepper_list_t *)pepper_compositor_get_seat_list(pepper_keyrouter->compositor);
				if (!pepper_list_empty(seat_list)) {
					pepper_list_for_each(seat, seat_list, link) {
						_pepper_keyrouter_key_send(pepper_keyrouter, seat, (struct wl_client *)info->data, key, state, time);
					}
				}
			}
		}
	}
	else {
		/* send key event to focus view if any */
		if (pepper_keyrouter->focus_view)
			pepper_keyboard_send_key(pepper_keyrouter->keyboard, pepper_keyrouter->focus_view, time, key, state);
		else
			PEPPER_TRACE("No focused view exists.\n", __FUNCTION__);
	}
}

static void
_pepper_keyrouter_remove_client_from_list(pepper_keyrouter_t *pepper_keyrouter, struct wl_client *client)
{
	int i;
	for (i = 0; i < KEYROUTER_MAX_KEYS; i++) {
		keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_EXCLUSIVE,
	                           i, (void *)client);
		keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE,
	                           i, (void *)client);
		keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_TOPMOST,
	                           i, (void *)client);
		keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_SHARED,
	                           i, (void *)client);
	}
}

static int
_pepper_keyrouter_add_client_to_list(pepper_keyrouter_t *pepper_keyrouter, struct wl_client *client)
{
	clients_data_t *cdata;

	pepper_list_for_each(cdata, &pepper_keyrouter->grabbed_clients, link) {
		if (cdata->client == client) return TIZEN_KEYROUTER_ERROR_NONE;
	}

	cdata = (clients_data_t *)calloc(1, sizeof(clients_data_t));
	if (!cdata)
	{
		PEPPER_ERROR("Failed to allocate memory !\n");
		return TIZEN_KEYROUTER_ERROR_NO_SYSTEM_RESOURCES;
	}

	cdata->client = client;
	pepper_list_init(&cdata->link);
	pepper_list_insert(&pepper_keyrouter->grabbed_clients, &cdata->link);

	return TIZEN_KEYROUTER_ERROR_NONE;
}

static void
_pepper_keyrouter_cb_keygrab_set(struct wl_client *client,
                                         struct wl_resource *resource,
                                         struct wl_resource *surface,
                                         uint32_t key,
                                         uint32_t mode)
{
	int res = TIZEN_KEYROUTER_ERROR_NONE;
	pepper_keyrouter_t *pepper_keyrouter = NULL;
	pepper_bool_t ret;

	pepper_keyrouter = (pepper_keyrouter_t *)wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_keyrouter, goto notify, "Invalid pepper_keyrouter_t\n");

	ret = _pepper_keyrouter_util_do_privilege_check(pepper_keyrouter, client, mode, key);
	if (!ret) {
		res = TIZEN_KEYROUTER_ERROR_NO_PERMISSION;
		goto notify;
	}

	res = keyrouter_grab_key(pepper_keyrouter->keyrouter, mode, key, (void *)client);

	if (res == TIZEN_KEYROUTER_ERROR_NONE)
		res = _pepper_keyrouter_add_client_to_list(pepper_keyrouter, client);

notify:
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, mode, res);
}

static void
_pepper_keyrouter_cb_keygrab_unset(struct wl_client *client,
                                           struct wl_resource *resource,
                                           struct wl_resource *surface,
                                           uint32_t key)
{
	int res = TIZEN_KEYROUTER_ERROR_NONE;
	pepper_keyrouter_t *pepper_keyrouter = NULL;
	pepper_bool_t ret;

	pepper_keyrouter = (pepper_keyrouter_t *)wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_keyrouter, goto notify, "Invalid pepper_keyrouter_t\n");

	/* ungrab TOP POSITION grab first, this grab mode is not check privilege */
	keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_TOPMOST,
	                           key, (void *)client);

	ret = _pepper_keyrouter_util_do_privilege_check(pepper_keyrouter, client, TIZEN_KEYROUTER_MODE_NONE, key);
	if (!ret) {
		res = TIZEN_KEYROUTER_ERROR_NO_PERMISSION;
		goto notify;
	}

	keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_EXCLUSIVE,
	                           key, (void *)client);
	keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE,
	                           key, (void *)client);
	keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_TOPMOST,
	                           key, (void *)client);
	keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
	                           TIZEN_KEYROUTER_MODE_SHARED,
	                           key, (void *)client);

notify:
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, TIZEN_KEYROUTER_MODE_NONE, res);
}

static void
_pepper_keyrouter_cb_get_keygrab_status(struct wl_client *client,
                                                struct wl_resource *resource,
                                                struct wl_resource *surface,
                                                uint32_t key)
{
	tizen_keyrouter_send_keygrab_notify(resource, surface, key, TIZEN_KEYROUTER_MODE_NONE, TIZEN_KEYROUTER_ERROR_NO_PERMISSION);
}

static int
_pepper_keyrouter_array_length(const struct wl_array *array)
{
	int *data = NULL;
	int count = 0;

	wl_array_for_each(data, array) {
		count++;
	}

	return count;
}

static void
_pepper_keyrouter_cb_keygrab_set_list(struct wl_client *client,
                                              struct wl_resource *resource,
                                              struct wl_resource *surface,
                                              struct wl_array *grab_list)
{
	struct wl_array *return_list = NULL;
	grab_list_data_t *grab_data = NULL;
	int res = TIZEN_KEYROUTER_ERROR_NONE;
	pepper_keyrouter_t *pepper_keyrouter = NULL;
	pepper_bool_t ret;

	pepper_keyrouter = (pepper_keyrouter_t *)wl_resource_get_user_data(resource);

	PEPPER_CHECK(pepper_keyrouter, goto notify, "Invalid pepper_keyrouter_t\n");
	PEPPER_CHECK(grab_list, goto notify, "Please send valid grab_list\n");
	PEPPER_CHECK(((_pepper_keyrouter_array_length(grab_list) %3) == 0), goto notify,
	             "Invalid keycode and grab mode pair. Check arguments in a list\n");

	wl_array_for_each(grab_data, grab_list) {
		ret = _pepper_keyrouter_util_do_privilege_check(pepper_keyrouter, client, grab_data->mode, grab_data->key);
		if (!ret) {
			grab_data->err = TIZEN_KEYROUTER_ERROR_NO_PERMISSION;
		} else {
			res = keyrouter_grab_key(pepper_keyrouter->keyrouter, grab_data->mode, grab_data->key, (void *)client);
			grab_data->err = res;
			if (res == TIZEN_KEYROUTER_ERROR_NONE)
				res = _pepper_keyrouter_add_client_to_list(pepper_keyrouter, client);
		}
	}

	return_list = grab_list;
notify:
	tizen_keyrouter_send_keygrab_notify_list(resource, surface, return_list);
}

static void
_pepper_keyrouter_cb_keygrab_unset_list(struct wl_client *client,
                                                struct wl_resource *resource,
                                                struct wl_resource *surface,
                                                struct wl_array *ungrab_list)
{
	struct wl_array *return_list = NULL;
	ungrab_list_data_t *ungrab_data = NULL;
	pepper_keyrouter_t *pepper_keyrouter = NULL;
	pepper_bool_t ret;

	pepper_keyrouter = (pepper_keyrouter_t *)wl_resource_get_user_data(resource);

	PEPPER_CHECK(pepper_keyrouter, goto notify, "Invalid pepper_keyrouter_t\n");
	PEPPER_CHECK(((_pepper_keyrouter_array_length(ungrab_list) %3) == 0), goto notify,
	             "Invalid keycode and grab mode pair. Check arguments in a list\n");

	wl_array_for_each(ungrab_data, ungrab_list) {
		keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
		                            TIZEN_KEYROUTER_MODE_TOPMOST,
		                            ungrab_data->key, (void *)client);

		ret = _pepper_keyrouter_util_do_privilege_check(pepper_keyrouter, client, TIZEN_KEYROUTER_MODE_TOPMOST, ungrab_data->key);
		if (!ret) {
			ungrab_data->err = TIZEN_KEYROUTER_ERROR_NO_PERMISSION;
		} else {
			keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
			                            TIZEN_KEYROUTER_MODE_EXCLUSIVE,
			                            ungrab_data->key, (void *)client);

			keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
			                            TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE,
			                            ungrab_data->key, (void *)client);

			keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
			                            TIZEN_KEYROUTER_MODE_TOPMOST,
			                            ungrab_data->key, (void *)client);

			keyrouter_ungrab_key(pepper_keyrouter->keyrouter,
			                            TIZEN_KEYROUTER_MODE_SHARED,
			                            ungrab_data->key, (void *)client);

			ungrab_data->err = TIZEN_KEYROUTER_ERROR_NONE;
		}
	}

	return_list = ungrab_list;
notify:
	tizen_keyrouter_send_keygrab_notify_list(resource, surface, return_list);
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

static void
_pepper_keyrouter_cb_destory(struct wl_client *client,
							struct wl_resource *resource)
{
	wl_resource_destroy(resource);
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
   _pepper_keyrouter_cb_set_input_config,
   _pepper_keyrouter_cb_destory
};

/* tizen_keyrouter global object destroy function */
static void
_pepper_keyrouter_cb_resource_destory(struct wl_resource *resource)
{
	resources_data_t *rdata, *rtmp;
	clients_data_t *cdata, *ctmp;
	pepper_list_t *list;
	struct wl_client *client;
	pepper_keyrouter_t *pepper_keyrouter;

	PEPPER_CHECK(resource, return, "Invalid keyrouter resource\n");
	client = wl_resource_get_client(resource);
	PEPPER_CHECK(client, return, "Invalid client\n");
	pepper_keyrouter = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_keyrouter, return, "Invalid pepper_keyrouter_t\n");

	list = &pepper_keyrouter->grabbed_clients;
	if (!pepper_list_empty(list)) {
		pepper_list_for_each_safe(cdata, ctmp, list, link) {
			if (cdata->client == client) {
				_pepper_keyrouter_remove_client_from_list(pepper_keyrouter, client);
				pepper_list_remove(&cdata->link);
				free(cdata);
			}
		}
	}

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

	resource = wl_resource_create(client, &tizen_keyrouter_interface, MIN(version, 2), id);
	if (!resource) {
		PEPPER_ERROR("Failed to create resource ! (version :%d, id:%d)", version, id);
		wl_client_post_no_memory(client);
		return;
	}

	rdata = (resources_data_t *)calloc(1, sizeof(resources_data_t));
	if (!rdata)
	{
		PEPPER_ERROR("Failed to allocate memory !\n");

		wl_resource_destroy(resource);
		wl_client_post_no_memory(client);
		return;
	}

	rdata->resource = resource;
	pepper_list_init(&rdata->link);
	pepper_list_insert(&pepper_keyrouter->resources, &rdata->link);


	wl_resource_set_implementation(resource, &_pepper_keyrouter_implementation,
	                               pepper_keyrouter, _pepper_keyrouter_cb_resource_destory);
}

PEPPER_API void
pepper_keyrouter_event_handler(pepper_event_listener_t *listener,
                               pepper_object_t *object,
                               uint32_t id, void *info, void *data)
{
	pepper_input_event_t *event;
	pepper_keyrouter_t *pepper_keyrouter;
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)object;

	PEPPER_CHECK((id == PEPPER_EVENT_KEYBOARD_KEY),
	             return, "%d event is not handled by keyrouter\n", id);
	PEPPER_CHECK(info, return, "Invalid event\n");
	PEPPER_CHECK(data, return, "Invalid data. Please insert pepper_keyrouter\n");

	event = (pepper_input_event_t *)info;
	pepper_keyrouter = (pepper_keyrouter_t *)data;
	if (!pepper_keyrouter->keyboard)
		pepper_keyrouter_set_keyboard(pepper_keyrouter, keyboard);
	pepper_keyrouter_key_process(pepper_keyrouter, event->key, event->state, event->time);
}

PEPPER_API pepper_keyrouter_t *
pepper_keyrouter_create(pepper_compositor_t *compositor)
{
	struct wl_display *display = NULL;
	struct wl_global *global = NULL;
	pepper_keyrouter_t *pepper_keyrouter;
	pepper_bool_t ret;

	PEPPER_CHECK(compositor, return PEPPER_FALSE, "Invalid compositor\n");

	display = pepper_compositor_get_display(compositor);
	PEPPER_CHECK(display, return PEPPER_FALSE, "Failed to get wl_display from compositor\n");

	pepper_keyrouter = (pepper_keyrouter_t *)calloc(1, sizeof(pepper_keyrouter_t));
	PEPPER_CHECK(pepper_keyrouter, return PEPPER_FALSE, "Failed to allocate memory for keyrouter\n");
	pepper_keyrouter->display = display;
	pepper_keyrouter->compositor = compositor;

	pepper_list_init(&pepper_keyrouter->resources);
	pepper_list_init(&pepper_keyrouter->grabbed_clients);

	global = wl_global_create(display, &tizen_keyrouter_interface, 2, pepper_keyrouter, _pepper_keyrouter_cb_bind);
	PEPPER_CHECK(global, goto failed, "Failed to create wl_global for tizen_keyrouter\n");

	pepper_keyrouter->global = global;

	pepper_keyrouter->keyrouter = keyrouter_create();
	PEPPER_CHECK(pepper_keyrouter->keyrouter, goto failed, "Failed to create keyrouter\n");

	ret = _pepper_keyrouter_util_cynara_init(pepper_keyrouter);
	if (!ret) PEPPER_TRACE("cynara initialize is failed, process keyrouter without cynara\n");

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
	clients_data_t *cdata, *ctmp;

	PEPPER_CHECK(pepper_keyrouter, return, "Pepper keyrouter is not initialized\n");

	pepper_list_for_each_safe(cdata, ctmp, &pepper_keyrouter->grabbed_clients, link) {
		pepper_list_remove(&cdata->link);
		free(cdata);
	}

	pepper_list_for_each_safe(rdata, rtmp, &pepper_keyrouter->resources, link) {
		wl_resource_destroy(rdata->resource);
	}

	if (pepper_keyrouter->keyrouter) {
		keyrouter_destroy(pepper_keyrouter->keyrouter);
		pepper_keyrouter->keyrouter = NULL;
	}

	if (pepper_keyrouter->global)
		wl_global_destroy(pepper_keyrouter->global);

	_pepper_keyrouter_util_cynara_deinit(pepper_keyrouter);

	free(pepper_keyrouter);
	pepper_keyrouter = NULL;
}
