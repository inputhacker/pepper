#ifndef PEPPER_EVDEV_H
#define PEPPER_EVDEV_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_evdev_t *
pepper_evdev_create(pepper_compositor_t *compositor);

PEPPER_API void
pepper_evdev_destroy(pepper_evdev_t *evdev);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_EVDEV_H */
