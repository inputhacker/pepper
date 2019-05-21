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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <pepper.h>
#include <wayland-server.h>
#include <pepper-inotify.h>

#define MAX_CMDS	256

#define STDOUT_REDIR			"stdout"
#define STDERR_REDIR			"stderr"
#define EVENT_TRACE_ON			"event_trace_on"
#define EVENT_TRACE_OFF		"event_trace_off"
#define KEYGRAB_STATUS			"keygrab_status"
#define TOPVWINS			"topvwins"
#define CONNECT_CLIENTS		"connect_clients"
#define HELP_MSG			"help"

typedef struct
{
	pepper_compositor_t *compositor;
	pepper_inotify_t *inotify;
} headless_debug_t;

typedef void (*headless_debug_action_cb_t)(headless_debug_t *hd, void *data);

typedef struct
{
   const char *cmds;
   headless_debug_action_cb_t cb;
   headless_debug_action_cb_t disable_cb;
} headless_debug_action_t;

const static int KEY_DEBUG = 0xdeaddeb0;
extern void wl_debug_server_enable(int enable);

static void
_headless_debug_usage()
{
	fprintf(stdout, "Commands:\n\n");
	fprintf(stdout, "\t %s\n", EVENT_TRACE_ON);
	fprintf(stdout, "\t %s\n", EVENT_TRACE_OFF);
	fprintf(stdout, "\t %s\n", STDOUT_REDIR);
	fprintf(stdout, "\t %s\n", STDERR_REDIR);
	fprintf(stdout, "\t %s\n", KEYGRAB_STATUS);
	fprintf(stdout, "\t %s\n", TOPVWINS);
	fprintf(stdout, "\t %s\n", CONNECT_CLIENTS);
	fprintf(stdout, "\t %s\n", HELP_MSG);

	fprintf(stdout, "\nTo execute commands, just create/remove/update a file with the commands above.\n");
	fprintf(stdout, "Please refer to the following examples.\n\n");
	fprintf(stdout, "\t touch %s to enable event trace\n", EVENT_TRACE_ON);
	fprintf(stdout, "\t rm -f %s to disable event trace\n", EVENT_TRACE_ON);
	fprintf(stdout, "\t touch %s to disable event trace\n", EVENT_TRACE_OFF);
	fprintf(stdout, "\t touch stdout to redirect STDOUT to stdout.txt\n");
	fprintf(stdout, "\t touch stderr to redirect STDERR to stderr.txt\n");
}

static void
_headless_debug_event_trace_on(headless_debug_t *hdebug, void *data)
{
	(void) hdebug;
	(void) data;
	wl_debug_server_enable(1);
}

static void
_headless_debug_event_trace_off(headless_debug_t *hdebug, void *data)
{
	(void) hdebug;
	(void) data;
	wl_debug_server_enable(0);
}

static void
_headless_debug_dummy(headless_debug_t *hdebug, void *data)
{
	(void) hdebug;
	(void) data;
	_headless_debug_usage();
}

static void
_headless_debug_redir_stdout(headless_debug_t *hdebug, void *data)
{
	(void) hdebug;
	(void) data;

	static int fd = -1;

	if (fd >= 0)
		close(fd);

	fd = open("/run/pepper/stdout.txt", O_CREAT | O_WRONLY | O_APPEND, S_IWUSR | S_IWGRP);

	if (fd < 0) {
		PEPPER_TRACE("Failed to open stdout.txt (errno=%s)\n", strerror(errno));
		return;
	}

	dup2(fd, 1);
	PEPPER_TRACE("STDOUT has been redirected to stdout.txt.\n");
}

static void
_headless_debug_redir_stderr(headless_debug_t *hdebug, void *data)
{
	(void) hdebug;
	(void) data;

	static int fd = -1;

	if (fd >= 0)
		close(fd);

	fd = open("/run/pepper/stderr.txt", O_CREAT | O_WRONLY | O_APPEND, S_IWUSR | S_IWGRP);

	if (fd < 0) {
		PEPPER_TRACE("Failed to open stderr.txt (errno=%s)\n", strerror(errno));
		return;
	}

	dup2(fd, 2);
	PEPPER_TRACE("STDERR has been redirected to stderr.txt.\n");

}

static const headless_debug_action_t debug_actions[] =
{
	{ STDOUT_REDIR,  _headless_debug_redir_stdout, NULL },
	{ STDERR_REDIR,  _headless_debug_redir_stderr, NULL },
	{ EVENT_TRACE_ON,  _headless_debug_event_trace_on, _headless_debug_event_trace_off },
	{ EVENT_TRACE_OFF, _headless_debug_event_trace_off, NULL },
	{ KEYGRAB_STATUS, _headless_debug_dummy, NULL },
	{ TOPVWINS, _headless_debug_dummy, NULL },
	{ CONNECT_CLIENTS, _headless_debug_dummy, NULL },
	{ HELP_MSG, _headless_debug_dummy, NULL },
};

static void
_headless_debug_enable_action(headless_debug_t *hdebug, char *cmds)
{
	int n_actions = sizeof(debug_actions)/sizeof(debug_actions[0]);

	for(int n=0 ; n < n_actions ; n++) {
		if (!strncmp(cmds, debug_actions[n].cmds, MAX_CMDS)) {
			debug_actions[n].cb(hdebug, (void *)debug_actions[n].cmds);
			PEPPER_TRACE("[%s : %s]\n", __FUNCTION__, debug_actions[n].cmds);

			break;
		}
	}
}

static void
_headless_debug_disable_action(headless_debug_t *hdebug, char *cmds)
{
	int n_actions = sizeof(debug_actions)/sizeof(debug_actions[0]);

	for(int n=0 ; n < n_actions ; n++) {
		if (!strncmp(cmds, debug_actions[n].cmds, MAX_CMDS)) {
			if (debug_actions[n].disable_cb) {
				debug_actions[n].disable_cb(hdebug, (void *)debug_actions[n].cmds);
				PEPPER_TRACE("[%s : %s]\n", __FUNCTION__, debug_actions[n].cmds);
			}

			break;
		}
	}
}

static void
_trace_cb_handle_inotify_event(uint32_t type, pepper_inotify_event_t *ev, void *data)
{
	headless_debug_t *hdebug = data;
	char *file_name = pepper_inotify_event_name_get(ev);

	PEPPER_CHECK(hdebug, return, "Invalid headless debug instance\n");

	switch (type)
	{
		case PEPPER_INOTIFY_EVENT_TYPE_CREATE:
			_headless_debug_enable_action(hdebug, file_name);
			break;
		case PEPPER_INOTIFY_EVENT_TYPE_REMOVE:
			_headless_debug_disable_action(hdebug, file_name);
			break;
		case PEPPER_INOTIFY_EVENT_TYPE_MODIFY:
			_headless_debug_enable_action(hdebug, file_name);
			break;

		default:
			PEPPER_TRACE("[%s] Unhandled event type (%d)\n", __FUNCTION__, type);
			break;
	}
}

PEPPER_API void
headless_debug_deinit(pepper_compositor_t * compositor)
{
	headless_debug_t *hdebug = NULL;

	hdebug = (headless_debug_t *)pepper_object_get_user_data((pepper_object_t *) compositor, &KEY_DEBUG);
	PEPPER_CHECK(hdebug, return, "Failed to get headless debug instance\n");

	/* remove the directory watching already */
	if (hdebug->inotify)
		pepper_inotify_del(hdebug->inotify, "/run/pepper");

	/* remove inotify */
	pepper_inotify_destroy(hdebug->inotify);
	hdebug->inotify = NULL;

	pepper_object_set_user_data((pepper_object_t *)hdebug->compositor, &KEY_DEBUG, NULL, NULL);
	free(hdebug);
}

pepper_bool_t
headless_debug_init(pepper_compositor_t *compositor)
{
	headless_debug_t *hdebug = NULL;
	pepper_inotify_t *inotify = NULL;
	pepper_bool_t res = PEPPER_FALSE;

	hdebug = (headless_debug_t*)calloc(1, sizeof(headless_debug_t));
	PEPPER_CHECK(hdebug, goto error, "Failed to alloc for headless debug\n");
	hdebug->compositor = compositor;

	/* create inotify to watch file(s) for event trace */
	inotify = pepper_inotify_create(hdebug->compositor, _trace_cb_handle_inotify_event, hdebug);
	PEPPER_CHECK(inotify, goto error, "Failed to create inotify\n");

	/* add a directory for watching */
	res = pepper_inotify_add(inotify, "/run/pepper");
	PEPPER_CHECK(res, goto error, "Failed on pepper_inotify_add()\n");

	hdebug->inotify = inotify;

	PEPPER_TRACE("[%s] ... done\n", __FUNCTION__);

	pepper_object_set_user_data((pepper_object_t *)compositor, &KEY_DEBUG, hdebug, NULL);
	return PEPPER_TRUE;

error:
	headless_debug_deinit(compositor);

	return PEPPER_FALSE;
}
