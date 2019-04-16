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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pepper-output-backend.h>
#include "HL_UI_LED.h"

#define NUM_LED 12

typedef struct {
    pepper_compositor_t *compositor;
    pepper_output_t   *output;
    pepper_plane_t    *plane;

    int num_led;
    HL_UI_LED *ui_led;
}led_output_t;

static const int KEY_OUTPUT;

static void
led_output_destroy(void *o)
{
    led_output_t *output = (led_output_t *)o;
    PEPPER_TRACE("Output Destroy %p base %p\n", output, output->output);

    if (output->ui_led)
        HL_UI_LED_Close(output->ui_led);

    free(output);
}

static int32_t
led_output_get_subpixel_order(void *o)
{
	return 0;
}

static const char *
led_output_get_maker_name(void *o)
{
	return "PePPer LED";
}

static const char *
led_output_get_model_name(void *o)
{
	return "PePPer LED";
}

static int
led_output_get_mode_count(void *o)
{
	return 1;
}

static void
led_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    led_output_t *output = (led_output_t *)o;

    if (index != 0)
        return;

    mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    mode->w = output->num_led;
    mode->h = output->num_led;
    mode->refresh = 60000;
}

static pepper_bool_t
led_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
	return PEPPER_FALSE;
}

static void
led_output_assign_planes(void *o, const pepper_list_t *view_list)
{
}

static void
led_output_start_repaint_loop(void *o)
{
    led_output_t *output = (led_output_t *)o;
    struct timespec     ts;

    pepper_compositor_get_time(output->compositor, &ts);
    pepper_output_finish_frame(output->output, &ts);
}

static void
led_output_repaint(void *o, const pepper_list_t *plane_list)
{
}

static void
led_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
}

static void
led_output_flush_surface_damage(void *o, pepper_surface_t *surface,
								  pepper_bool_t *keep_buffer)
{
}

struct pepper_output_backend led_output_backend = {
	led_output_destroy,

	led_output_get_subpixel_order,
	led_output_get_maker_name,
	led_output_get_model_name,

	led_output_get_mode_count,
	led_output_get_mode,
	led_output_set_mode,

	led_output_assign_planes,
	led_output_start_repaint_loop,
	led_output_repaint,
	led_output_attach_surface,
	led_output_flush_surface_damage,
};

pepper_bool_t
pepper_output_led_init(pepper_compositor_t *compositor)
{
    led_output_t *output = (led_output_t*)calloc(sizeof(led_output_t), 1);

    PEPPER_TRACE("Output Init\n");

    if (!output) {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    output->num_led = NUM_LED;
    output->ui_led = HL_UI_LED_Init(output->num_led);
    if (!output->ui_led)
        PEPPER_ERROR("HL_UI_LED_Init() failed.\n");

    output->compositor = compositor;
    output->output = pepper_compositor_add_output(compositor,
                                                   &led_output_backend, "led_output",
                                                   output,  WL_OUTPUT_TRANSFORM_NORMAL, 1);
    PEPPER_CHECK(output->output, goto error, "pepper_compositor_add_output() failed.\n");

    output->plane = pepper_output_add_plane(output->output, NULL);
    PEPPER_CHECK(output->plane, goto error, "pepper_output_add_plane() failed.\n");

    pepper_object_set_user_data((pepper_object_t *)compositor,
                                 &KEY_OUTPUT, output, led_output_destroy);
    PEPPER_TRACE("\t Add Output %p, base %p\n", output, output->output);
    PEPPER_TRACE("\t Add Output %p, plane %p\n", output, output->plane);
    PEPPER_TRACE("\t Userdata %p\n", pepper_object_get_user_data((pepper_object_t *)compositor,&KEY_OUTPUT));
    return PEPPER_TRUE;

    error:
    if (output->ui_led)
        HL_UI_LED_Close(output->ui_led);

    if (output->output)
      pepper_output_destroy(output->output);

    if (output)
      free(output);
    return PEPPER_FALSE;
}

void
pepper_output_led_deinit(pepper_compositor_t *compositor)
{
    PEPPER_TRACE("Output Deinit ... DONE\n");
}
