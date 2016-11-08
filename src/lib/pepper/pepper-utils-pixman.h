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

#ifndef PEPPER_UTILS_PIXMAN_H
#define PEPPER_UTILS_PXIMAN_H

#include <pixman.h>
#include <pepper-utils.h>

/**
 * Get pixman format from pepper_format_t
 *
 * @param format    pepper_format
 *
 * If use this inline function, you should include libpixman library.
 */
static inline pixman_format_code_t
pepper_get_pixman_format(pepper_format_t format)
{
	switch (format) {
	case PEPPER_FORMAT_ARGB8888:
		return PIXMAN_a8r8g8b8;
	case PEPPER_FORMAT_XRGB8888:
		return PIXMAN_x8r8g8b8;
	case PEPPER_FORMAT_RGB888:
		return PIXMAN_r8g8b8;
	case PEPPER_FORMAT_RGB565:
		return PIXMAN_r5g6b5;
	case PEPPER_FORMAT_ABGR8888:
		return PIXMAN_a8b8g8r8;
	case PEPPER_FORMAT_XBGR8888:
		return PIXMAN_x8b8g8r8;
	case PEPPER_FORMAT_BGR888:
		return PIXMAN_b8g8r8;
	case PEPPER_FORMAT_BGR565:
		return PIXMAN_b5g6r5;
	case PEPPER_FORMAT_ALPHA:
		return PIXMAN_a8;
	default:
		break;
	}

	return (pixman_format_code_t)0;
}

#endif
