#ifndef PEPPER_RENDER_H
#define PEPPER_RENDER_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_renderer      pepper_renderer_t;
typedef struct pepper_render_target pepper_render_target_t;

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer);

PEPPER_API void
pepper_render_target_destroy(pepper_render_target_t *target);

PEPPER_API pepper_bool_t
pepper_renderer_set_target(pepper_renderer_t *renderer, pepper_render_target_t *target);

PEPPER_API pepper_render_target_t *
pepper_renderer_get_target(pepper_renderer_t *renderer);

PEPPER_API pepper_bool_t
pepper_renderer_attach_surface(pepper_renderer_t *renderer,
                               pepper_object_t *surface, int *w, int *h);

PEPPER_API pepper_bool_t
pepper_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_object_t *surface);

PEPPER_API void
pepper_renderer_repaint_output(pepper_renderer_t *renderer, pepper_object_t *output);

PEPPER_API pepper_bool_t
pepper_renderer_read_pixels(pepper_renderer_t *renderer, int x, int y, int w, int h,
                            void *pixels, pepper_format_t format);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_RENDER_H */
