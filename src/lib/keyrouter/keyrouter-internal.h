#ifndef KEYROUTER_INTERNAL_H
#define KEYROUTER_INTERNAL_H

#include <unistd.h>
#include <config.h>

#include "keyrouter.h"

typedef struct keyrouter_grabbed keyrouter_grabbed_t;

struct keyrouter {
	pepper_compositor_t        *compositor;
	keyrouter_grabbed_t *hard_keys;
};

struct keyrouter_grabbed {
	int keycode;
	struct {
		pepper_list_t excl;
		pepper_list_t or_excl;
		pepper_list_t top;
		pepper_list_t shared;
	} grab;
	pepper_list_t pressed;
};

#endif /* KEYROUTER_INTERNAL_H */
