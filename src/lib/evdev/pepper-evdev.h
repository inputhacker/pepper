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

#ifndef PEPPER_EVDEV_H
#define PEPPER_EVDEV_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API int
pepper_evdev_device_probe(pepper_evdev_t *evdev, uint32_t caps);

PEPPER_API pepper_evdev_t *
pepper_evdev_create(pepper_compositor_t *compositor);

PEPPER_API void
pepper_evdev_destroy(pepper_evdev_t *evdev);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_EVDEV_H */
