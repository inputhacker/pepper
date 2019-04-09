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

#ifndef DEVICEMGR_INTERNAL_H
#define DEVICEMGR_INTERNAL_H

#include <unistd.h>
#include <config.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>

#include <pepper-input-backend.h>
#include "devicemgr.h"

struct devicemgr_device {
	char name[UINPUT_MAX_NAME_SIZE + 1];
	pepper_input_device_t *input_device;
};

struct devicemgr {
	pepper_compositor_t *compositor;
	pepper_seat_t *seat;
	devicemgr_device_t *keyboard;
};

#endif /* DEVICEMGR_INTERNAL_H */
