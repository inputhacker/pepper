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
#include <config.h>

#include "pepper-pixman-renderer.h"
#include "pepper-render-internal.h"
#include <pepper-output-backend.h>
#include <pepper-utils-pixman.h>

#ifdef HAVE_TBM
#include <tbm_surface.h>
#include <wayland-tbm-server.h>
#endif

typedef struct pixman_renderer      pixman_renderer_t;
typedef struct pixman_surface_state pixman_surface_state_t;
typedef struct pixman_render_target pixman_render_target_t;

struct pixman_render_target {
	pepper_render_target_t  base;
	pixman_image_t         *image;
};

struct pixman_renderer {
	pepper_renderer_t   base;
	pixman_image_t     *background;
};

struct pixman_surface_state {
	pixman_renderer_t       *renderer;

	pepper_surface_t        *surface;
	pepper_event_listener_t *surface_destroy_listener;

	pepper_buffer_t         *buffer;
	pepper_event_listener_t *buffer_destroy_listener;
	int                      buffer_width, buffer_height;

	pixman_image_t          *image;

};

static void
pixman_renderer_destroy(pepper_renderer_t *renderer)
{
	pixman_renderer_t *pr = (pixman_renderer_t *)renderer;

	if (pr->background)
		pixman_image_unref(pr->background);

	free(renderer);
}

static void
surface_state_destroy_image(pixman_surface_state_t *state)
{
	if (state->image) {
		pixman_image_unref(state->image);
		state->image = NULL;
	}
}

static void
surface_state_release_buffer(pixman_surface_state_t *state)
{
	surface_state_destroy_image(state);

	if (state->buffer) {
		pepper_event_listener_remove(state->buffer_destroy_listener);
		state->buffer = NULL;
	}
}

static void
surface_state_handle_buffer_destroy(pepper_event_listener_t    *listener,
									pepper_object_t            *object,
									uint32_t                    id,
									void                       *info,
									void                       *data)
{
	pixman_surface_state_t *state = data;
	surface_state_release_buffer(state);
}

static void
surface_state_handle_surface_destroy(pepper_event_listener_t    *listener,
									 pepper_object_t            *object,
									 uint32_t                    id,
									 void                       *info,
									 void                       *data)
{
	pixman_surface_state_t *state = data;

	surface_state_release_buffer(state);
	pepper_event_listener_remove(state->surface_destroy_listener);
	pepper_object_set_user_data((pepper_object_t *)state->surface, state->renderer,
								NULL, NULL);
	free(state);
}

static void
surface_state_begin_access(pixman_surface_state_t *state)
{
	struct wl_shm_buffer *shm_buffer;
#ifdef HAVE_TBM
	tbm_surface_h tbm_surface;
#endif

	shm_buffer = wl_shm_buffer_get(pepper_buffer_get_resource(state->buffer));
	if (shm_buffer) {
		wl_shm_buffer_begin_access(shm_buffer);
		return;
	}
#ifdef HAVE_TBM
	tbm_surface = wayland_tbm_server_get_surface(NULL,
				  pepper_buffer_get_resource(state->buffer));
	if (tbm_surface) {
		tbm_surface_info_s tmp;
		tbm_surface_map(tbm_surface, TBM_SURF_OPTION_READ, &tmp);
		return;
	}
#endif
}

static void
surface_state_end_access(pixman_surface_state_t *state)
{
	struct wl_shm_buffer *shm_buffer;
#ifdef HAVE_TBM
	tbm_surface_h tbm_surface;
#endif

	shm_buffer = wl_shm_buffer_get(pepper_buffer_get_resource(state->buffer));
	if (shm_buffer) {
		wl_shm_buffer_end_access(shm_buffer);
		return;
	}
#ifdef HAVE_TBM
	tbm_surface = wayland_tbm_server_get_surface(NULL,
				  pepper_buffer_get_resource(state->buffer));
	if (tbm_surface) {
		tbm_surface_unmap(tbm_surface);
		return;
	}
#endif
}


static pixman_surface_state_t *
get_surface_state(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
	pixman_surface_state_t *state = pepper_object_get_user_data((
										pepper_object_t *)surface, renderer);

	if (!state) {
		state = calloc(1, sizeof(pixman_surface_state_t));
		if (!state)
			return NULL;

		state->surface = surface;
		state->surface_destroy_listener =
			pepper_object_add_event_listener((pepper_object_t *)surface,
											 PEPPER_EVENT_OBJECT_DESTROY, 0,
											 surface_state_handle_surface_destroy, state);

		pepper_object_set_user_data((pepper_object_t *)surface, renderer, state, NULL);
	}

	return state;
}

static pepper_bool_t
surface_state_attach_shm(pixman_surface_state_t *state, pepper_buffer_t *buffer)
{
	struct wl_shm_buffer   *shm_buffer = wl_shm_buffer_get(
			pepper_buffer_get_resource(buffer));
	pixman_format_code_t    format;
	int                     w, h;
	pixman_image_t         *image;

	if (!shm_buffer)
		return PEPPER_FALSE;

	switch (wl_shm_buffer_get_format(shm_buffer)) {
	case WL_SHM_FORMAT_XRGB8888:
		format = PIXMAN_x8r8g8b8;
		break;
	case WL_SHM_FORMAT_ARGB8888:
		format = PIXMAN_a8r8g8b8;
		break;
	case WL_SHM_FORMAT_RGB565:
		format = PIXMAN_r5g6b5;
		break;
	default:
		return PEPPER_FALSE;
	}

	w = wl_shm_buffer_get_width(shm_buffer);
	h = wl_shm_buffer_get_height(shm_buffer);

	image = pixman_image_create_bits(format, w, h,
									 wl_shm_buffer_get_data(shm_buffer),
									 wl_shm_buffer_get_stride(shm_buffer));

	if (!image)
		return PEPPER_FALSE;

	state->buffer_width = w;
	state->buffer_height = h;
	state->image = image;
	//state->shm_buffer = shm_buffer;

	return PEPPER_TRUE;;
}

#ifdef HAVE_TBM
static pepper_bool_t
surface_state_attach_tbm(pixman_surface_state_t *state, pepper_buffer_t *buffer)
{
	tbm_surface_h   tbm_surface = wayland_tbm_server_get_surface(NULL,
								  pepper_buffer_get_resource(buffer));
	tbm_surface_info_s info;
	pixman_format_code_t    format;
	int                     w, h;
	pixman_image_t         *image;

	if (!tbm_surface)
		return PEPPER_FALSE;

	switch (tbm_surface_get_format(tbm_surface)) {
	case TBM_FORMAT_XRGB8888:
		format = PIXMAN_x8r8g8b8;
		break;
	case TBM_FORMAT_ARGB8888:
		format = PIXMAN_a8r8g8b8;
		break;
	case TBM_FORMAT_RGB565:
		format = PIXMAN_r5g6b5;
		break;
	default:
		return PEPPER_FALSE;
	}

	tbm_surface_get_info(tbm_surface, &info);
	if (info.num_planes != 1)
		return PEPPER_FALSE;

	w = info.width;
	h = info.height;
	image = pixman_image_create_bits(format, w, h,
									 (uint32_t *)info.planes[0].ptr,
									 info.planes[0].stride);

	if (!image)
		return PEPPER_FALSE;

	state->buffer_width = w;
	state->buffer_height = h;
	state->image = image;
	//state->shm_buffer = shm_buffer;

	return PEPPER_TRUE;;
}
#endif

static pepper_bool_t
pixman_renderer_attach_surface(pepper_renderer_t *renderer,
							   pepper_surface_t *surface,
							   int *w, int *h)
{
	pixman_surface_state_t *state = get_surface_state(renderer, surface);
	pepper_buffer_t        *buffer = pepper_surface_get_buffer(surface);

	surface_state_release_buffer(state);

	if (!buffer) {
		state->buffer_width = 0;
		state->buffer_height = 0;

		goto done;
	}

	if (surface_state_attach_shm(state, buffer))
		goto done;

#ifdef HAVE_TBM
	if (surface_state_attach_tbm(state, buffer))
		goto done;
#endif

	PEPPER_ERROR("Not supported buffer type.\n");
	return PEPPER_FALSE;

done:
	state->buffer = buffer;

	if (state->buffer) {
		state->buffer_destroy_listener =
			pepper_object_add_event_listener((pepper_object_t *)buffer,
											 PEPPER_EVENT_OBJECT_DESTROY, 0,
											 surface_state_handle_buffer_destroy, state);
	}

	*w = state->buffer_width;
	*h = state->buffer_height;

	return PEPPER_TRUE;
}

static pepper_bool_t
pixman_renderer_flush_surface_damage(pepper_renderer_t *renderer,
									 pepper_surface_t *surface)
{
	return PEPPER_TRUE;
}

static pepper_bool_t
pixman_renderer_read_pixels(pepper_renderer_t *renderer,
							int x, int y, int w, int h,
							void *pixels, pepper_format_t format)
{
	pixman_image_t         *src = ((pixman_render_target_t *)
								   renderer->target)->image;
	pixman_image_t         *dst;
	pixman_format_code_t    pixman_format;
	int                     stride;

	if (!src)
		return PEPPER_FALSE;

	pixman_format = pepper_get_pixman_format(format);

	if (!pixman_format) {
		/* PEPPER_ERROR("Invalid format.\n"); */
		return PEPPER_FALSE;
	}

	stride = (PEPPER_FORMAT_BPP(format) / 8) * w;
	dst = pixman_image_create_bits(pixman_format, w, h, pixels, stride);

	if (!dst) {
		/* PEPPER_ERROR("Failed to create pixman image.\n"); */
		return PEPPER_FALSE;
	}

	pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst, x, y, 0, 0, 0, 0, w, h);
	pixman_image_unref(dst);

	return PEPPER_TRUE;
}

static void
pixman_transform_from_pepper_mat4(pixman_transform_t *transform,
								  pepper_mat4_t *mat)
{
	transform->matrix[0][0] = pixman_double_to_fixed(mat->m[0]);
	transform->matrix[0][1] = pixman_double_to_fixed(mat->m[4]);
	transform->matrix[0][2] = pixman_double_to_fixed(mat->m[12]);
	transform->matrix[1][0] = pixman_double_to_fixed(mat->m[1]);
	transform->matrix[1][1] = pixman_double_to_fixed(mat->m[5]);
	transform->matrix[1][2] = pixman_double_to_fixed(mat->m[13]);
	transform->matrix[2][0] = pixman_double_to_fixed(mat->m[3]);
	transform->matrix[2][1] = pixman_double_to_fixed(mat->m[7]);
	transform->matrix[2][2] = pixman_double_to_fixed(mat->m[15]);
}

static void
repaint_view(pepper_renderer_t *renderer, pepper_output_t *output,
			 pepper_render_item_t *node, pepper_region_t *damage)
{
	int32_t                  x, y, w, h, scale;
	pepper_surface_t        *surface = pepper_view_get_surface(node->view);

	pixman_render_target_t  *target = (pixman_render_target_t *)renderer->target;
	pepper_region_t        repaint, repaint_surface;
	pepper_region_t        surface_blend, *surface_opaque;
	pixman_surface_state_t  *ps = get_surface_state(renderer,
								  pepper_view_get_surface(node->view));

	pepper_region_init(&repaint);
	pepper_region_intersect(&repaint, &node->visible_region, damage);

	if (pepper_region_not_empty(&repaint)) {
		pixman_transform_t  trans;
		pixman_filter_t     filter;

		if (node->transform.flags <= PEPPER_MATRIX_TRANSLATE) {
			pixman_transform_init_translate(&trans,
											-pixman_double_to_fixed(node->transform.m[12]),
											-pixman_double_to_fixed(node->transform.m[13]));
			filter = PIXMAN_FILTER_NEAREST;
		} else {
			pixman_transform_from_pepper_mat4(&trans, &node->inverse);
			filter = PIXMAN_FILTER_BILINEAR;
		}

		pepper_surface_get_buffer_offset(surface, &x, &y);
		pepper_surface_get_size(surface, &w, &h);
		pixman_transform_translate(&trans, NULL,
								   pixman_int_to_fixed(x), pixman_int_to_fixed(y));

		switch (pepper_surface_get_buffer_transform(surface)) {
		case WL_OUTPUT_TRANSFORM_FLIPPED:
		case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		case WL_OUTPUT_TRANSFORM_FLIPPED_270:
			pixman_transform_scale(&trans, NULL, pixman_int_to_fixed(-1),
								   pixman_int_to_fixed(1));
			pixman_transform_translate(&trans, NULL, pixman_int_to_fixed(w), 0);
			break;
		}

		switch (pepper_surface_get_buffer_transform(surface)) {
		case WL_OUTPUT_TRANSFORM_NORMAL:
		case WL_OUTPUT_TRANSFORM_FLIPPED:
			break;
		case WL_OUTPUT_TRANSFORM_90:
		case WL_OUTPUT_TRANSFORM_FLIPPED_90:
			pixman_transform_rotate(&trans, NULL, 0, pixman_fixed_1);
			pixman_transform_translate(&trans, NULL, pixman_int_to_fixed(h), 0);
			break;
		case WL_OUTPUT_TRANSFORM_180:
		case WL_OUTPUT_TRANSFORM_FLIPPED_180:
			pixman_transform_rotate(&trans, NULL, -pixman_fixed_1, 0);
			pixman_transform_translate(&trans, NULL,
									   pixman_int_to_fixed(w),
									   pixman_int_to_fixed(h));
			break;
		case WL_OUTPUT_TRANSFORM_270:
		case WL_OUTPUT_TRANSFORM_FLIPPED_270:
			pixman_transform_rotate(&trans, NULL, 0, -pixman_fixed_1);
			pixman_transform_translate(&trans, NULL, 0, pixman_int_to_fixed(w));
			break;
		}

		scale = pepper_surface_get_buffer_scale(surface);
		pixman_transform_scale(&trans, NULL,
							   pixman_int_to_fixed(scale),
							   pixman_int_to_fixed(scale));

		pixman_image_set_transform(ps->image, &trans);
		pixman_image_set_filter(ps->image, filter, NULL, 0);

		pepper_region_init(&repaint_surface);
		pepper_region_init_rect(&surface_blend, 0, 0, w, h);

		if (node->transform.flags <= PEPPER_MATRIX_TRANSLATE) {
			surface_opaque = pepper_surface_get_opaque_region(surface);
			pepper_region_subtract(&surface_blend, &surface_blend, surface_opaque);

			if (pepper_region_not_empty(surface_opaque)) {
				pepper_region_translate(surface_opaque,
										  (int)node->transform.m[12], (int)node->transform.m[13]);
				pepper_region_global_to_output(surface_opaque, output);
				pepper_region_intersect(&repaint_surface, &repaint, surface_opaque);
				pixman_image_set_clip_region32(target->image, (pixman_region32_t*)&repaint_surface);

				surface_state_begin_access(ps);
				pixman_image_composite32(PIXMAN_OP_SRC, ps->image, NULL, target->image,
										 0, 0, /* src_x, src_y */
										 0, 0, /* mask_x, mask_y */
										 0, 0, /* dest_x, dest_y */
										 pixman_image_get_width(target->image),
										 pixman_image_get_height(target->image));
				surface_state_end_access(ps);
			}

			if (pepper_region_not_empty(&surface_blend)) {
				pepper_region_translate(&surface_blend,
										  (int)node->transform.m[12], (int)node->transform.m[13]);
				pepper_region_global_to_output(&surface_blend, output);
				pepper_region_intersect(&repaint_surface, &repaint, &surface_blend);
				pixman_image_set_clip_region32(target->image, (pixman_region32_t*)&repaint_surface);

				surface_state_begin_access(ps);
				pixman_image_composite32(PIXMAN_OP_OVER, ps->image, NULL, target->image,
										 0, 0, /* src_x, src_y */
										 0, 0, /* mask_x, mask_y */
										 0, 0, /* dest_x, dest_y */
										 pixman_image_get_width(target->image),
										 pixman_image_get_height(target->image));
				surface_state_end_access(ps);
			}
		} else {
			pepper_region_translate(&surface_blend,
									  (int)node->transform.m[12], (int)node->transform.m[13]);
			pepper_region_global_to_output(&surface_blend, output);
			pepper_region_intersect(&repaint_surface, &repaint, &surface_blend);
			pixman_image_set_clip_region32(target->image, (pixman_region32_t*)&repaint_surface);

			surface_state_begin_access(ps);
			pixman_image_composite32(PIXMAN_OP_OVER, ps->image, NULL, target->image,
									 0, 0, /* src_x, src_y */
									 0, 0, /* mask_x, mask_y */
									 0, 0, /* dest_x, dest_y */
									 pixman_image_get_width(target->image),
									 pixman_image_get_height(target->image));
			surface_state_end_access(ps);
		}

		pepper_region_fini(&repaint_surface);
		pepper_region_fini(&surface_blend);
	}

	pepper_region_fini(&repaint);
}

static void
clear_background(pixman_renderer_t *renderer, pepper_region_t *damage)
{
	pixman_render_target_t *target = (pixman_render_target_t *)
									 renderer->base.target;

	pixman_image_set_clip_region32(target->image, (pixman_region32_t*)damage);
	pixman_image_composite32(PIXMAN_OP_SRC, renderer->background, NULL,
							 target->image,
							 0, 0, 0, 0, 0, 0,
							 pixman_image_get_width(target->image),
							 pixman_image_get_height(target->image));
}

static void
pixman_renderer_repaint_output(pepper_renderer_t *renderer,
							   pepper_output_t *output,
							   const pepper_list_t *render_list,
							   pepper_region_t *damage)
{
	if (pepper_region_not_empty(damage)) {
		pepper_list_t       *l;
		pixman_renderer_t   *pr = (pixman_renderer_t *)renderer;

		if (pr->background)
			clear_background((pixman_renderer_t *)renderer, damage);

		pepper_list_for_each_list_reverse(l, render_list)
		repaint_view(renderer, output, (pepper_render_item_t *)l->item, damage);
	}
}

PEPPER_API pepper_renderer_t *
pepper_pixman_renderer_create(pepper_compositor_t *compositor)
{
	pixman_renderer_t  *renderer;
	const char         *env;

	renderer = calloc(1, sizeof(pixman_renderer_t));
	if (!renderer)
		return NULL;

	renderer->base.compositor = compositor;

	env = getenv("PEPPER_RENDER_CLEAR_BACKGROUND");

	if (env && atoi(env) == 1) {
		pixman_color_t bg_color = { 0x0000, 0x0000, 0x0000, 0xffff };
		renderer->background = pixman_image_create_solid_fill(&bg_color);
	}

	/* Backend functions. */
	renderer->base.destroy              = pixman_renderer_destroy;
	renderer->base.attach_surface       = pixman_renderer_attach_surface;
	renderer->base.flush_surface_damage = pixman_renderer_flush_surface_damage;
	renderer->base.read_pixels          = pixman_renderer_read_pixels;
	renderer->base.repaint_output       = pixman_renderer_repaint_output;

	return &renderer->base;
}

static void
pixman_render_target_destroy(pepper_render_target_t *target)
{
	pixman_render_target_t *pt = (pixman_render_target_t *)target;

	if (pt->image)
		pixman_image_unref(pt->image);

	free(target);
}

PEPPER_API pepper_render_target_t *
pepper_pixman_renderer_create_target(pepper_format_t format, void *pixels,
									 int stride, int width, int height)
{
	pixman_render_target_t *target;
	pixman_format_code_t    pixman_format;

	target = calloc(1, sizeof(pixman_render_target_t));
	if (!target)
		return NULL;

	target->base.destroy    = pixman_render_target_destroy;

	pixman_format = pepper_get_pixman_format(format);
	if (!pixman_format)
		goto error;

	target->image = pixman_image_create_bits(pixman_format, width, height, pixels,
					stride);
	if (!target->image)
		goto error;

	target->base.destroy = pixman_render_target_destroy;
	return &target->base;

error:
	if (target)
		free(target);

	return NULL;
}

PEPPER_API pepper_render_target_t *
pepper_pixman_renderer_create_target_for_image(pixman_image_t *image)
{
	pixman_render_target_t *target;

	target = calloc(1, sizeof(pixman_render_target_t));
	if (!target)
		return NULL;

	pixman_image_ref(image);
	target->image = image;
	target->base.destroy = pixman_render_target_destroy;

	return &target->base;
}
