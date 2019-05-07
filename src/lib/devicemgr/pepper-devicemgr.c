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

#include "pepper-devicemgr.h"
#include "pepper-internal.h"
#include <tizen-extension-server-protocol.h>
#include <pepper-xkb.h>
#ifdef HAVE_CYNARA
#include <cynara-session.h>
#include <cynara-client.h>
#include <cynara-creds-socket.h>
#include <sys/smack.h>
#include <stdio.h>
#include <stdarg.h>
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct pepper_devicemgr_resource pepper_devicemgr_resource_t;

struct pepper_devicemgr {
	struct wl_global *global;
	struct wl_display *display;
	pepper_compositor_t *compositor;
	pepper_seat_t *seat;
	pepper_bool_t xkb_enabled;
	pepper_xkb_info_t *xkb_info;
	pepper_list_t *keymap_list;

	pepper_event_listener_t *listener_input_device_add;
	pepper_event_listener_t *listener_seat_keyboard_add;
	pepper_event_listener_t *listener_keyboard_keymap_update;

	pepper_list_t resources;

	devicemgr_t *devicemgr;
	int ref;
#ifdef HAVE_CYNARA
	cynara *p_cynara;
#endif
};

struct pepper_devicemgr_resource {
	struct wl_resource *resource;
	pepper_bool_t init;
	pepper_list_t link;
};

#ifdef HAVE_CYNARA
static void
_pepper_devicemgr_util_cynara_log(int err, const char *fmt, ...)
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
_pepper_devicemgr_util_cynara_init(pepper_devicemgr_t *pepper_devicemgr)
{
#ifdef HAVE_CYNARA
	int ret;
	if (pepper_devicemgr->p_cynara) return PEPPER_TRUE;

	ret = cynara_initialize(&pepper_devicemgr->p_cynara, NULL);
	if (CYNARA_API_SUCCESS != ret) {
		_pepper_devicemgr_util_cynara_log(ret, "cynara_initialize");
		return PEPPER_FALSE;
	}
#endif
	return PEPPER_TRUE;
}

static void
_pepper_devicemgr_util_cynara_deinit(pepper_devicemgr_t *pepper_devicemgr)
{
#ifdef HAVE_CYNARA
	if (pepper_devicemgr->p_cynara) cynara_finish(pepper_devicemgr->p_cynara);
#else
	;
#endif
}

static pepper_bool_t
_pepper_devicemgr_util_do_privilege_check(pepper_devicemgr_t *pepper_devicemgr, struct wl_client *client, const char *rule)
{
	pepper_bool_t res = PEPPER_TRUE;
#ifdef HAVE_CYNARA
	int ret, retry_cnt = 0, len = 0;
	char *clientSmack = NULL, *client_session = NULL, uid2[16]={0, };
	static pepper_bool_t retried = PEPPER_FALSE;
	pid_t pid = 0;
	uid_t uid = 0;
	gid_t gid = 0;

	res = PEPPER_FALSE;

	/* Top position grab is always allowed. This mode do not need privilege.*/
	if (!client) return PEPPER_FALSE;

	/* If initialize cynara is failed, allow keygrabs regardless of the previlege permition. */
	if (pepper_devicemgr->p_cynara == NULL) {
		if (retried == PEPPER_FALSE) {
			retried = PEPPER_TRUE;
			for(retry_cnt = 0; retry_cnt < 5; retry_cnt++) {
				PEPPER_TRACE("Retry cynara initialize: %d\n", retry_cnt + 1);

				ret = cynara_initialize(&pepper_devicemgr->p_cynara, NULL);
				if (CYNARA_API_SUCCESS != ret) {
					_pepper_devicemgr_util_cynara_log(ret, "cynara_initialize retry..");
					pepper_devicemgr->p_cynara = NULL;
				} else {
					PEPPER_TRACE("Success cynara initialize to try %d times\n", retry_cnt + 1);
					break;
				}
			}
		}
		if (!pepper_devicemgr->p_cynara) return PEPPER_TRUE;
	}

	wl_client_get_credentials(client, &pid, &uid, &gid);

	len = smack_new_label_from_process((int)pid, &clientSmack);
	if (len <= 0) goto finish;

	snprintf(uid2, 15, "%d", (int)uid);
	client_session = cynara_session_from_pid(pid);

	ret = cynara_check(pepper_devicemgr->p_cynara, clientSmack, client_session, uid2, rule);
	if (CYNARA_API_ACCESS_ALLOWED == ret) {
		res = PEPPER_TRUE;
		PEPPER_TRACE("Success to check cynara, clientSmack: %s client_session: %s, uid2: %s\n", clientSmack, client_session, uid2);
	} else {
		_pepper_devicemgr_util_cynara_log(ret, "rule: %s, clientsmack: %s, pid: %d", rule, clientSmack, pid);
	}

finish:
	if (client_session) free(client_session);
	if (clientSmack) free(clientSmack);
#endif
	return res;
}

static void
_pepper_devicemgr_handle_keyboard_keymap_update(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)info;
	pepper_devicemgr_t *pepper_devicemgr = (pepper_devicemgr_t *)data;

	pepper_devicemgr->xkb_info = keyboard->xkb_info;
}

static void
_pepper_devicemgr_handle_seat_keyboard_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_keyboard_t *keyboard = (pepper_keyboard_t *)info;
	pepper_devicemgr_t *pepper_devicemgr = (pepper_devicemgr_t *)data;

	pepper_devicemgr->xkb_info = keyboard->xkb_info;

	pepper_devicemgr->listener_keyboard_keymap_update =
		pepper_object_add_event_listener((pepper_object_t *)keyboard,
			PEPPER_EVENT_KEYBOARD_KEYMAP_UPDATE, 0,
			_pepper_devicemgr_handle_keyboard_keymap_update, pepper_devicemgr);
}

static void
_pepper_devicemgr_handle_input_device_add(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id, void *info, void *data)
{
	pepper_input_device_t *device = (pepper_input_device_t *)info;
	pepper_devicemgr_t *pepper_devicemgr = (pepper_devicemgr_t *)data;

	if (pepper_input_device_get_caps(device) & WL_SEAT_CAPABILITY_KEYBOARD)
		devicemgr_input_generator_keyboard_set(pepper_devicemgr->devicemgr, device);
}

static void
_pepper_devicemgr_cb_block_events(struct wl_client *client, struct wl_resource *resource,
                             uint32_t serial, uint32_t clas, uint32_t duration)
{
	tizen_input_device_manager_send_error(resource, TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES);
}

static void
_pepper_devicemgr_cb_unblock_events(struct wl_client *client, struct wl_resource *resource,
                             uint32_t serial)
{
	tizen_input_device_manager_send_error(resource, TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES);
}

static int
_pepper_devicemgr_init_generator(pepper_devicemgr_t *pepper_devicemgr, struct wl_resource *resource, uint32_t clas, const char *name)
{
	int ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES;
	pepper_devicemgr_resource_t *rdata;

	ret = devicemgr_input_generator_init(pepper_devicemgr->devicemgr, clas, name);
	PEPPER_CHECK(ret == TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE, return ret, "Failed to init input generator\n");

	pepper_list_for_each(rdata, &pepper_devicemgr->resources, link) {
		if (rdata->resource == resource) {
			if (rdata->init == PEPPER_FALSE) {
				rdata->init = PEPPER_TRUE;
				pepper_devicemgr->ref++;
			}
			break;
		}
	}

	return ret;
}

static int
_pepper_devicemgr_deinit_generator(pepper_devicemgr_t *pepper_devicemgr, struct wl_resource *resource)
{
	int ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE;
	pepper_devicemgr_resource_t *rdata;

	pepper_list_for_each(rdata, &pepper_devicemgr->resources, link) {
		if (rdata->resource == resource) {
			if (rdata->init == PEPPER_TRUE) {
				rdata->init = PEPPER_FALSE;
				pepper_devicemgr->ref--;
				if (pepper_devicemgr->ref < 0) pepper_devicemgr->ref = 0;
				break;
			} else {
				return ret;
			}
		}
	}

	if (pepper_devicemgr->ref <= 0) {
		ret = devicemgr_input_generator_deinit(pepper_devicemgr->devicemgr);
		PEPPER_CHECK(ret == TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE, return ret, "Failed to init input generator\n");
	}

	return ret;
}

static void
_pepper_devicemgr_cb_init_generator(struct wl_client *client, struct wl_resource *resource, uint32_t clas)
{
	pepper_devicemgr_t *pepper_devicemgr;
	int ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES;
	pepper_bool_t res;

	pepper_devicemgr = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_devicemgr, goto failed, "pepper_devicemgr is not set\n");
	PEPPER_CHECK(pepper_devicemgr->devicemgr, goto failed, "devicemgr is not created\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_PERMISSION;
	res = _pepper_devicemgr_util_do_privilege_check(pepper_devicemgr, client, "http://tizen.org/privilege/inputgenerator");
	PEPPER_CHECK(res == PEPPER_TRUE, goto failed, "Current client has no permission to input generate\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_INVALID_PARAMETER;
	PEPPER_CHECK(clas == TIZEN_INPUT_DEVICE_MANAGER_CLAS_KEYBOARD, goto failed,
		"only support keyboard device. (requested: 0x%x)\n", clas);

	ret = _pepper_devicemgr_init_generator(pepper_devicemgr, resource, clas, "Input Generator");
	PEPPER_CHECK(ret == TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE, goto failed, "Failed to init input generator\n");

failed:
	tizen_input_device_manager_send_error(resource, ret);
}

static void
_pepper_devicemgr_cb_init_generator_with_name(struct wl_client *client, struct wl_resource *resource, uint32_t clas, const char *name)
{
	pepper_devicemgr_t *pepper_devicemgr;
	int ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES;
	pepper_bool_t res;

	pepper_devicemgr = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_devicemgr, goto failed, "pepper_devicemgr is not set\n");
	PEPPER_CHECK(pepper_devicemgr->devicemgr, goto failed, "devicemgr is not created\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_PERMISSION;
	res = _pepper_devicemgr_util_do_privilege_check(pepper_devicemgr, client, "http://tizen.org/privilege/inputgenerator");
	PEPPER_CHECK(res == PEPPER_TRUE, goto failed, "Current client has no permission to input generate\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_INVALID_PARAMETER;
	PEPPER_CHECK(clas == TIZEN_INPUT_DEVICE_MANAGER_CLAS_KEYBOARD, goto failed,
		"only support keyboard device. (requested: 0x%x)\n", clas);

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_INVALID_PARAMETER;
	PEPPER_CHECK(name, goto failed, "no name for device\n");

	ret = _pepper_devicemgr_init_generator(pepper_devicemgr, resource, clas, name);
	PEPPER_CHECK(ret == TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE, goto failed, "Failed to init input generator\n");

failed:
	tizen_input_device_manager_send_error(resource, ret);
}

static void
_pepper_devicemgr_cb_deinit_generator(struct wl_client *client, struct wl_resource *resource, uint32_t clas)
{
	pepper_devicemgr_t *pepper_devicemgr;
	int ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES;
	pepper_bool_t res;

	pepper_devicemgr = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_devicemgr, goto failed, "pepper_devicemgr is not set\n");
	PEPPER_CHECK(pepper_devicemgr->devicemgr, goto failed, "devicemgr is not created\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_PERMISSION;
	res = _pepper_devicemgr_util_do_privilege_check(pepper_devicemgr, client, "http://tizen.org/privilege/inputgenerator");
	PEPPER_CHECK(res == PEPPER_TRUE, goto failed, "Current client has no permission to input generate\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_INVALID_PARAMETER;
	PEPPER_CHECK(clas == TIZEN_INPUT_DEVICE_MANAGER_CLAS_KEYBOARD, goto failed,
		"only support keyboard device. (requested: 0x%x)\n", clas);

	ret = _pepper_devicemgr_deinit_generator(pepper_devicemgr, resource);
	PEPPER_CHECK(ret == TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE, goto failed, "Failed to init input generator\n");

failed:
	tizen_input_device_manager_send_error(resource, ret);
}

PEPPER_API void
pepper_devicemgr_keymap_set(pepper_devicemgr_t *pepper_devicemgr, pepper_list_t *list)
{
	PEPPER_CHECK(pepper_devicemgr || list, return, "Please insert correct data\n");
	if (pepper_devicemgr->keymap_list) return;

	pepper_devicemgr->keymap_list = list;
}

static int
_pepper_devicemgr_keyname_to_keycode(pepper_list_t *list, const char *name)
{
	pepper_devicemgr_keymap_data_t *data;

	if (!strncmp(name, "Keycode-", sizeof("Keycode-")-1)) {
		return atoi(name + 8);
	}
	else if (list && !pepper_list_empty(list)) {
		pepper_list_for_each(data, list, link) {
			if (data) {
				if (!strncmp(data->name, name, UINPUT_MAX_NAME_SIZE)) {
					return data->keycode;
				}
			}
		}
	}

	return 0;
}

static void
_pepper_devicemgr_cb_generate_key(struct wl_client *client, struct wl_resource *resource,
                                const char *keyname, uint32_t pressed)
{
	pepper_devicemgr_t *pepper_devicemgr;
	int ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES;
	int keycode = 0;
	pepper_bool_t res;

	pepper_devicemgr = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_devicemgr, goto failed, "pepper_devicemgr is not set\n");
	PEPPER_CHECK(pepper_devicemgr->devicemgr, goto failed, "devicemgr is not created\n");

	ret = TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_PERMISSION;
	res = _pepper_devicemgr_util_do_privilege_check(pepper_devicemgr, client, "http://tizen.org/privilege/inputgenerator");
	PEPPER_CHECK(res == PEPPER_TRUE, goto failed, "Current client has no permission to input generate\n");

	if (pepper_devicemgr->xkb_info) {
		keycode = pepper_xkb_info_keyname_to_keycode(pepper_devicemgr->xkb_info, keyname);
	}
	else
		keycode = _pepper_devicemgr_keyname_to_keycode(pepper_devicemgr->keymap_list, keyname);

	ret = devicemgr_input_generator_generate_key(pepper_devicemgr->devicemgr, keycode, pressed);
	PEPPER_CHECK(ret == TIZEN_INPUT_DEVICE_MANAGER_ERROR_NONE, goto failed, "Failed to generate key(name: %s, code: %d), ret: %d\n", keyname, keycode, ret);

failed:
	tizen_input_device_manager_send_error(resource, ret);
}

static void
_pepper_devicemgr_cb_generate_pointer(struct wl_client *client, struct wl_resource *resource,
                                    uint32_t type, uint32_t x, uint32_t y, uint32_t button)
{
	tizen_input_device_manager_send_error(resource, TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES);
}

static void
_pepper_devicemgr_cb_generate_touch(struct wl_client *client, struct wl_resource *resource,
                                   uint32_t type, uint32_t x, uint32_t y, uint32_t finger)
{
	tizen_input_device_manager_send_error(resource, TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES);
}

static void
_pepper_devicemgr_cb_pointer_warp(struct wl_client *client, struct wl_resource *resource, struct wl_resource *surface, wl_fixed_t x, wl_fixed_t y)
{
	tizen_input_device_manager_send_error(resource, TIZEN_INPUT_DEVICE_MANAGER_ERROR_NO_SYSTEM_RESOURCES);
}

static void
_pepper_devicemgr_cb_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}


static const struct tizen_input_device_manager_interface _pepper_devmgr_implementation = {
   _pepper_devicemgr_cb_block_events,
   _pepper_devicemgr_cb_unblock_events,
   _pepper_devicemgr_cb_init_generator,
   _pepper_devicemgr_cb_deinit_generator,
   _pepper_devicemgr_cb_generate_key,
   _pepper_devicemgr_cb_generate_pointer,
   _pepper_devicemgr_cb_generate_touch,
   _pepper_devicemgr_cb_pointer_warp,
   _pepper_devicemgr_cb_init_generator_with_name,
   _pepper_devicemgr_cb_destroy,
};

/* tizen_devicemgr global object bind function */
static void
_pepper_devicemgr_cb_unbind(struct wl_resource *resource)
{
	pepper_devicemgr_t *pepper_devicemgr;

	pepper_devicemgr = wl_resource_get_user_data(resource);
	PEPPER_CHECK(pepper_devicemgr, return, "Invalid pepper_devicemgr_t\n");

	_pepper_devicemgr_deinit_generator(pepper_devicemgr, resource);
}

static void
_pepper_devicemgr_cb_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
	pepper_devicemgr_resource_t *rdata;
	pepper_devicemgr_t *pepper_devicemgr;

	pepper_devicemgr = (pepper_devicemgr_t *)data;
	PEPPER_CHECK(client, return, "Invalid client\n");
	PEPPER_CHECK(pepper_devicemgr, return, "Invalid pepper_devicemgr_t\n");

	resource = wl_resource_create(client, &tizen_input_device_manager_interface, 2, id);
	if (!resource) {
		PEPPER_ERROR("Failed to create resource ! (version :%d, id:%d)", version, id);
		wl_client_post_no_memory(client);
		return;
	}

	rdata = (pepper_devicemgr_resource_t *)calloc(1, sizeof(pepper_devicemgr_resource_t));
	if (!rdata) {
		PEPPER_ERROR("Failed to allocate memory !\n");

		wl_resource_destroy(resource);
		wl_client_post_no_memory(client);
		return;
	}

	rdata->resource = resource;
	pepper_list_init(&rdata->link);
	pepper_list_insert(&pepper_devicemgr->resources, &rdata->link);

	wl_resource_set_implementation(resource, &_pepper_devmgr_implementation,
	                               pepper_devicemgr, _pepper_devicemgr_cb_unbind);
}

PEPPER_API void
pepper_devicemgr_xkb_enable(pepper_devicemgr_t *pepper_devicemgr)
{
	if (pepper_devicemgr->xkb_enabled) return;

	pepper_devicemgr->xkb_enabled = PEPPER_TRUE;
	pepper_devicemgr->listener_seat_keyboard_add = pepper_object_add_event_listener((pepper_object_t *)pepper_devicemgr->seat,
		PEPPER_EVENT_SEAT_KEYBOARD_ADD,
		0, _pepper_devicemgr_handle_seat_keyboard_add, pepper_devicemgr);
}

PEPPER_API pepper_devicemgr_t *
pepper_devicemgr_create(pepper_compositor_t *compositor, pepper_seat_t *seat)
{
	struct wl_display *display = NULL;
	struct wl_global *global = NULL;
	pepper_devicemgr_t *pepper_devicemgr;
	pepper_bool_t ret;

	PEPPER_CHECK(compositor, return PEPPER_FALSE, "Invalid compositor\n");

	display = pepper_compositor_get_display(compositor);
	PEPPER_CHECK(display, return PEPPER_FALSE, "Failed to get wl_display from compositor\n");

	pepper_devicemgr = (pepper_devicemgr_t *)calloc(1, sizeof(pepper_devicemgr_t));
	PEPPER_CHECK(pepper_devicemgr, return PEPPER_FALSE, "Failed to allocate memory for devicemgr\n");
	pepper_devicemgr->display = display;
	pepper_devicemgr->compositor = compositor;
	pepper_devicemgr->seat = seat;

	pepper_devicemgr->xkb_enabled = PEPPER_FALSE;
	pepper_devicemgr->xkb_info = NULL;

	pepper_devicemgr->listener_input_device_add = pepper_object_add_event_listener((pepper_object_t *)pepper_devicemgr->compositor,
		PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
		0, _pepper_devicemgr_handle_input_device_add, pepper_devicemgr);

	pepper_list_init(&pepper_devicemgr->resources);

	global = wl_global_create(display, &tizen_input_device_manager_interface, 2, pepper_devicemgr, _pepper_devicemgr_cb_bind);
	PEPPER_CHECK(global, goto failed, "Failed to create wl_global for tizen_devicemgr\n");

	pepper_devicemgr->devicemgr = devicemgr_create(compositor, seat);
	PEPPER_CHECK(pepper_devicemgr->devicemgr, goto failed, "Failed to create devicemgr\n");

	ret = _pepper_devicemgr_util_cynara_init(pepper_devicemgr);
	if (!ret) PEPPER_TRACE("cynara initialize is failed. process devicemgr without cynara\n");

	return pepper_devicemgr;

failed:
	if (pepper_devicemgr) {
		if (pepper_devicemgr->devicemgr) {
			devicemgr_destroy(pepper_devicemgr->devicemgr);
			pepper_devicemgr->devicemgr = NULL;
		}
		free(pepper_devicemgr);
	}

	return NULL;
}

PEPPER_API void
pepper_devicemgr_destroy(pepper_devicemgr_t *pepper_devicemgr)
{
	pepper_devicemgr_resource_t *rdata, *rtmp;

	PEPPER_CHECK(pepper_devicemgr, return, "Pepper devicemgr is not initialized\n");

	_pepper_devicemgr_util_cynara_deinit(pepper_devicemgr);

	pepper_list_for_each_safe(rdata, rtmp, &pepper_devicemgr->resources, link) {
		wl_resource_destroy(rdata->resource);
		pepper_list_remove(&rdata->link);
		free(rdata);
	}

	if (pepper_devicemgr->devicemgr) {
		devicemgr_destroy(pepper_devicemgr->devicemgr);
		pepper_devicemgr->devicemgr = NULL;
	}

	if (pepper_devicemgr->global)
		wl_global_destroy(pepper_devicemgr->global);

	free(pepper_devicemgr);
	pepper_devicemgr = NULL;
}
