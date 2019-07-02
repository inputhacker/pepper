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

#include "pepper-utils.h"

#ifdef HAVE_CYNARA
#include <cynara-session.h>
#include <cynara-client.h>
#include <cynara-creds-socket.h>
#include <sys/smack.h>
#endif

#ifdef HAVE_CYNARA
static cynara *g_cynara = NULL;
static int g_cynara_refcount = 0;
static int g_cynara_init_count = 0;

static void
_pepper_security_log_print(int err, const char *fmt, ...)
{
	int ret;
	va_list args;

	const int CYNARA_BUFSIZE = 128;
	char buf[CYNARA_BUFSIZE] = "\0", tmp[CYNARA_BUFSIZE + CYNARA_BUFSIZE] = "\0";

	if (fmt)	{
		va_start(args, fmt);
		vsnprintf(tmp, CYNARA_BUFSIZE + CYNARA_BUFSIZE, fmt, args);
		va_end(args);
	}

	ret = cynara_strerror(err, buf, CYNARA_BUFSIZE);

	if (ret != CYNARA_API_SUCCESS)	{
		PEPPER_ERROR("Failed to get cynara_strerror. error : %d (error log about %s: %d)\n", ret, tmp, err);
		return;
	}

	PEPPER_ERROR("%s is failed. (%s)\n", tmp, buf);
}
#endif

PEPPER_API pepper_bool_t
pepper_security_privilege_check(pid_t pid, uid_t uid, const char *privilege)
{
#ifdef HAVE_CYNARA
	pepper_bool_t res = PEPPER_FALSE;

	PEPPER_CHECK(privilege, return PEPPER_FALSE, "Invalid privilege was given.\n");

	/* If cynara_initialize() has been (retried) and failed, we suppose that cynara is not available. */
	/* Then we return PEPPER_TRUE as if there is no security check available. */
	if (!g_cynara && g_cynara_init_count)
		return PEPPER_TRUE;

	PEPPER_CHECK(g_cynara, return PEPPER_FALSE, "Pepper-security has not been initialized.\n");

	char *client_smack = NULL;
	char *client_session = NULL;
	char uid_str[16] = { 0, };
	int len = -1;
	int ret = -1;

	ret = smack_new_label_from_process((int)pid, &client_smack);
	PEPPER_CHECK(ret > 0, goto finish, "");

	snprintf(uid_str, 15, "%d", (int)uid);

	client_session = cynara_session_from_pid(pid);
	PEPPER_CHECK(client_session, finish, "");

	ret = cynara_check(g_cynara,
	                  client_smack,
	                  client_session,
	                  uid_str,
	                  privilege);

	if (ret == CYNARA_API_ACCESS_ALLOWED)
		res = PEPPER_TRUE;
	else
		_pepper_security_log_print(ret, "rule: %s, client_smack: %s, pid: %d", rule, client_smack, pid);

finish:
	PEPPER_TRACE("Privilege Check For '%s' %s pid:%u uid:%u client_smack:%s(len:%d) client_session:%s ret:%d",
					NULL, privilege, res ? "SUCCESS" : "FAIL", pid, uid,
					client_smack ? client_smack : "N/A", len,
					client_session ? client_session: "N/A", ret);

	if (client_session)
		free(client_session);
	if (client_smack)
		free(client_smack);

	return res;
#else
	return PEPPER_TRUE;
#endif
}

PEPPER_API int
pepper_security_init(void)
{
#ifdef HAVE_CYNARA
	int ret;
	int retry_cnt = 0;
	static pepper_bool_t retried = PEPPER_FALSE;

	++g_cynara_init_count;
	if (++g_cynara_refcount != 1)
		return g_cynara_refcount;

	if (!g_cynara && PEPPER_FALSE == retried)	{
		retried = PEPPER_TRUE;

		for (retry_cnt = 0; retry_cnt < 5; retry_cnt++)	{

			PEPPER_TRACE("Retry cynara initialize: %d\n", retry_cnt + 1);

			ret = cynara_initialize(&g_cynara, NULL);

			if (CYNARA_API_SUCCESS == ret)	{
				PEPPER_TRACE("Succeed to initialize cynara !\n");
				return 1;
			}

			_pepper_security_log_print(ret, "cynara_initialize");
			g_cynara = NULL;
		}
	}

	PEPPER_ERROR("Failed to initialize pepper_security ! (error:%d, retry_cnt=%d)\n", ret, retry_cnt);
	--g_cynara_refcount;

	return 0;
#endif
	return 1;
}

PEPPER_API int
pepper_security_deinit(void)
{
#ifdef HAVE_CYNARA
	if (g_cynara_refcount < 1)
	{
		PEPPER_ERROR("%s called without pepper_security_init");
		return 0;
	}

	if (--g_cynara_refcount != 0)
		return 1;

	if (g_cynara)	{
		cynara_finish(g_cynara);
		g_cynara = NULL;
	}
#endif
	return 1;
}

