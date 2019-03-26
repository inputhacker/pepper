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

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <config.h>

#ifdef HAVE_DLOG
# include <dlog.h>
#  ifdef LOG_TAG
#   undef LOG_TAG
#  endif
# define LOG_TAG "PEPPER"
#endif

static FILE *pepper_log_file;
static int pepper_log_verbosity = 3;
#ifdef HAVE_DLOG
static pepper_bool_t pepper_dlog_enable = PEPPER_FALSE;
#endif
static int cached_tm_mday = -1;

static void __attribute__ ((constructor))
before_main(void)
{
	pepper_log_file = stdout;
}

static int
pepper_print_timestamp(void)
{
	struct timeval tv;
	struct tm *brokendown_time = NULL;
	char string[128];

	gettimeofday(&tv, NULL);

	brokendown_time = calloc(1, sizeof(*brokendown_time));
	if (!brokendown_time)
		return fprintf(pepper_log_file, "failed to calloc for brokendown_time\n");

	if (!localtime_r(&tv.tv_sec, brokendown_time)) {
		free(brokendown_time);
		return fprintf(pepper_log_file, "[(NULL)localtime] ");
	}

	if (brokendown_time->tm_mday != cached_tm_mday) {
		strftime(string, sizeof string, "%Y-%m-%d %Z", brokendown_time);
		fprintf(pepper_log_file, "Date: %s\n", string);

		cached_tm_mday = brokendown_time->tm_mday;
	}

	strftime(string, sizeof string, "%H:%M:%S", brokendown_time);

	free(brokendown_time);

	return fprintf(pepper_log_file, "[%s.%03li] ", string, tv.tv_usec / 1000);
}

static int
pepper_print_domain(const char *log_domain)
{
	if (log_domain == NULL)
		return fprintf(pepper_log_file, "UNKNOWN: ");
	else
		return fprintf(pepper_log_file, "%s: ", log_domain);
}

static int
pepper_vlog(const char *format, int level, va_list ap)
{
#ifdef HAVE_DLOG
	int dlog_level = DLOG_INFO;

	if (pepper_dlog_enable)
	{
		if (level == PEPPER_LOG_LEVEL_DEBUG)
			dlog_level = DLOG_DEBUG;
		else if (level == PEPPER_LOG_LEVEL_ERROR)
			dlog_level = DLOG_ERROR;

		return dlog_vprint(dlog_level, LOG_TAG, format, ap);
	}
#endif
	return vfprintf(pepper_log_file, format, ap);
}

PEPPER_API int
pepper_log(const char *domain, int level, const char *format, ...)
{
	int l = 0;
	va_list argp;

	if (level > pepper_log_verbosity || level < 0)
		return 0;
#ifdef HAVE_DLOG
	if (!pepper_dlog_enable)
	{
		l = pepper_print_timestamp();
		l += pepper_print_domain(domain);
	}
#else
	l = pepper_print_timestamp();
	l += pepper_print_domain(domain);
#endif

	va_start(argp, format);
	l += pepper_vlog(format, level, argp);
	va_end(argp);

	return l;
}

PEPPER_API void
pepper_log_dlog_enable(pepper_bool_t enabled)
{
#ifdef HAVE_DLOG
	pepper_dlog_enable = enabled;
#else
	;
#endif
}
