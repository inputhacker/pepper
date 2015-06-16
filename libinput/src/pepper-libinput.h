#ifndef PEPPER_LIBINPUT_H
#define PEPPER_LIBINPUT_H

#include <libudev.h>
#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_libinput   pepper_libinput_t;

PEPPER_API pepper_libinput_t *
pepper_libinput_create(pepper_object_t *compositor, struct udev *udev);

PEPPER_API void
pepper_libinput_destroy(pepper_libinput_t *input);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_LIBINPUT_H */
