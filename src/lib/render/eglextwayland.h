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

/* EGL extensions used by Pepper, copied from weston's weston-egl-ext.h */

#ifndef EGL_EXT_WAYLAND_H
#define EGL_EXT_WAYLAND_H

#ifndef EGL_WL_bind_wayland_display
#define EGL_WL_bind_wayland_display 1

struct wl_display;

#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglBindWaylandDisplayWL(EGLDisplay dpy,
		struct wl_display *display);
EGLAPI EGLBoolean EGLAPIENTRY eglUnbindWaylandDisplayWL(EGLDisplay dpy,
		struct wl_display *display);
#endif
typedef EGLBoolean (EGLAPIENTRYP PFNEGLBINDWAYLANDDISPLAYWL) (EGLDisplay dpy,
		struct wl_display *display);
typedef EGLBoolean (EGLAPIENTRYP PFNEGLUNBINDWAYLANDDISPLAYWL) (EGLDisplay dpy,
		struct wl_display *display);
#endif

/*
 * This is a little different to the tests shipped with EGL implementations,
 * which wrap the entire thing in #ifndef EGL_WL_bind_wayland_display, then go
 * on to define both BindWaylandDisplay and QueryWaylandBuffer.
 *
 * Unfortunately, some implementations (particularly the version of Mesa shipped
 * in Ubuntu 12.04) define EGL_WL_bind_wayland_display, but then only provide
 * prototypes for (Un)BindWaylandDisplay, completely omitting
 * QueryWaylandBuffer.
 *
 * Detect this, and provide our own definitions if necessary.
 */
#ifndef EGL_WAYLAND_BUFFER_WL
#define EGL_WAYLAND_BUFFER_WL		0x31D5 /* eglCreateImageKHR target */
#define EGL_WAYLAND_PLANE_WL		0x31D6 /* eglCreateImageKHR target */

#define EGL_TEXTURE_Y_U_V_WL            0x31D7
#define EGL_TEXTURE_Y_UV_WL             0x31D8
#define EGL_TEXTURE_Y_XUXV_WL           0x31D9
#define EGL_TEXTURE_EXTERNAL_WL         0x31DA

struct wl_resource;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglQueryWaylandBufferWL(EGLDisplay dpy,
		struct wl_resource *buffer, EGLint attribute, EGLint *value);
#endif
typedef EGLBoolean (EGLAPIENTRYP PFNEGLQUERYWAYLANDBUFFERWL) (EGLDisplay dpy,
		struct wl_resource *buffer, EGLint attribute, EGLint *value);
#endif

#ifndef EGL_WL_create_wayland_buffer_from_image
#define EGL_WL_create_wayland_buffer_from_image 1

#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI struct wl_buffer *EGLAPIENTRY eglCreateWaylandBufferFromImageWL(
	EGLDisplay dpy, EGLImageKHR image);
#endif
typedef struct wl_buffer *(EGLAPIENTRYP PFNEGLCREATEWAYLANDBUFFERFROMIMAGEWL) (
	EGLDisplay dpy, EGLImageKHR image);
#endif

#ifndef EGL_TEXTURE_EXTERNAL_WL
#define EGL_TEXTURE_EXTERNAL_WL         0x31DA
#endif

#ifndef EGL_BUFFER_AGE_EXT
#define EGL_BUFFER_AGE_EXT              0x313D
#endif

#ifndef EGL_WAYLAND_Y_INVERTED_WL
#define EGL_WAYLAND_Y_INVERTED_WL		0x31DB /* eglQueryWaylandBufferWL attribute */
#endif

#ifndef EGL_NATIVE_SURFACE_TIZEN
#define EGL_NATIVE_SURFACE_TIZEN		0x32A1
#endif

/* Mesas gl2ext.h and probably Khronos upstream defined
 * GL_EXT_unpack_subimage with non _EXT suffixed GL_UNPACK_* tokens.
 * In case we're using that mess, manually define the _EXT versions
 * of the tokens here.*/
#if defined(GL_EXT_unpack_subimage) && !defined(GL_UNPACK_ROW_LENGTH_EXT)
#define GL_UNPACK_ROW_LENGTH_EXT                                0x0CF2
#define GL_UNPACK_SKIP_ROWS_EXT                                 0x0CF3
#define GL_UNPACK_SKIP_PIXELS_EXT                               0x0CF4
#endif


#endif
