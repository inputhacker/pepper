#ifndef KEYROUTER_H
#define KEYROUTER_H

#include <pepper.h>
#include <tizen-extension-server-protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYROUTER_MAX_KEYS 512

typedef struct keyrouter keyrouter_t;
typedef struct keyrouter_key_info keyrouter_key_info_t;

struct keyrouter_key_info {
	void *data;
	pepper_list_t link;
};

PEPPER_API keyrouter_t *keyrouter_create(void);
PEPPER_API void keyrouter_destroy(keyrouter_t *keyrouter);

#ifdef __cplusplus
}
#endif

#endif /* KEYROUTER_H */

