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

#include "pepper-internal.h"

static void
region_resource_destroy_handler(struct wl_resource *resource)
{
	pepper_wl_region_t *region = wl_resource_get_user_data(resource);
	pepper_wl_region_destroy(region);
}

static void
region_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
region_add(struct wl_client *client, struct wl_resource *resource,
		   int32_t x, int32_t y, int32_t w, int32_t h)
{
	pepper_wl_region_t *region = wl_resource_get_user_data(resource);
	pepper_region_union_rect(&region->region, &region->region,
							   x, y, w, h);
}

static void
region_subtract(struct wl_client *client, struct wl_resource *resource,
				int32_t x, int32_t y, int32_t w, int32_t h)
{
	pepper_wl_region_t    *region = wl_resource_get_user_data(resource);
	pepper_region_t   rect;

	pepper_region_init_rect(&rect, x, y, w, h);
	pepper_region_subtract(&region->region, &region->region, &rect);
	pepper_region_fini(&rect);
}

static const struct wl_region_interface region_implementation = {
	region_destroy,
	region_add,
	region_subtract,
};

pepper_wl_region_t *
pepper_wl_region_create(pepper_compositor_t   *compositor,
					 struct wl_client      *client,
					 struct wl_resource    *resource,
					 uint32_t               id)
{
	pepper_wl_region_t *region = calloc(1, sizeof(pepper_wl_region_t));
	PEPPER_CHECK(region, return NULL, "calloc(0 failed.\n");

	region->resource = wl_resource_create(client, &wl_region_interface, 1, id);
	PEPPER_CHECK(region->resource, goto error, "wl_resource_create() failed\n");

	region->compositor = compositor;
	wl_resource_set_implementation(region->resource, &region_implementation,
								   region, region_resource_destroy_handler);

	region->link.item = region;
	pepper_list_insert(&compositor->region_list, &region->link);
	pepper_region_init(&region->region);

	return region;

error:
	if (region)
		free(region);

	return NULL;
}

void
pepper_wl_region_destroy(pepper_wl_region_t *region)
{
	pepper_region_fini(&region->region);
	pepper_list_remove(&region->link);
	free(region);
}

static inline void
add_bbox_point(double *box, int x, int y, const pepper_mat4_t *matrix)
{
	pepper_vec2_t v = { x, y };

	pepper_mat4_transform_vec2(matrix, &v);

	box[0] = PEPPER_MIN(box[0], v.x);
	box[1] = PEPPER_MIN(box[1], v.y);
	box[2] = PEPPER_MAX(box[2], v.x);
	box[3] = PEPPER_MAX(box[3], v.y);
}

static inline void
transform_bounding_box(pepper_box_t *box, const pepper_mat4_t *matrix)
{
	double          b[4] = { HUGE_VAL, HUGE_VAL, -HUGE_VAL, -HUGE_VAL };

	add_bbox_point(b, box->x1, box->y1, matrix);
	add_bbox_point(b, box->x2, box->y1, matrix);
	add_bbox_point(b, box->x2, box->y2, matrix);
	add_bbox_point(b, box->x1, box->y2, matrix);

	box->x1 = floor(b[0]);
	box->y1 = floor(b[1]);
	box->x2 = ceil(b[2]);
	box->y2 = ceil(b[3]);
}

void
pepper_transform_region(pepper_region_t *region,
							   const pepper_mat4_t *matrix)

{
	pepper_region_t   result;
	pepper_box_t     *rects;
	int                 i, num_rects;

	pepper_region_init(&result);
	rects = pepper_region_rectangles(region, &num_rects);

	for (i = 0; i < num_rects; i++) {
		pepper_box_t box = rects[i];

		transform_bounding_box(&box, matrix);
		pepper_region_union_rect(&result, &result,
								   box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
	}

	pepper_region_copy(region, &result);
	pepper_region_fini(&result);
}
