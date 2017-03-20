#include "keyrouter-internal.h"

PEPPER_API keyrouter_t *
keyrouter_create(void)
{
	keyrouter_t *keyrouter = NULL;
	int i;

	keyrouter = (keyrouter_t *)calloc(1, sizeof(keyrouter_t));
	PEPPER_CHECK(keyrouter, return NULL, "keyrouter allocation failed.\n");

	/* FIXME: Who defined max keycode? */
	keyrouter->hard_keys = (keyrouter_grabbed_t *)calloc(KEYROUTER_MAX_KEYS, sizeof(keyrouter_grabbed_t));
	PEPPER_CHECK(keyrouter->hard_keys, goto alloc_failed, "keyrouter allocation failed.\n");

	for (i = 0; i < KEYROUTER_MAX_KEYS; i++) {
		/* Enable all of keys to grab */
		//keyrouter->hard_keys[i].keycode = i;
		pepper_list_init(&keyrouter->hard_keys[i].grab.excl);
		pepper_list_init(&keyrouter->hard_keys[i].grab.or_excl);
		pepper_list_init(&keyrouter->hard_keys[i].grab.top);
		pepper_list_init(&keyrouter->hard_keys[i].grab.shared);
		pepper_list_init(&keyrouter->hard_keys[i].pressed);
	}

	return keyrouter;

alloc_failed:
	if (keyrouter) {
		if (keyrouter->hard_keys) {
			free(keyrouter->hard_keys);
			keyrouter->hard_keys = NULL;
		}
		free(keyrouter);
		keyrouter = NULL;
	}
	return NULL;
}

static void
_keyrouter_list_free(pepper_list_t *list)
{
	keyrouter_key_info_t *info, *tmp;

	if (pepper_list_empty(list)) return;

	pepper_list_for_each_safe(info, tmp, list, link) {
		pepper_list_remove(&info->link);
		free(info);
		info = NULL;
	}
}

PEPPER_API void
keyrouter_destroy(keyrouter_t *keyrouter)
{
	int i;

	PEPPER_CHECK(keyrouter, return, "Invalid keyrouter resource.\n");
	PEPPER_CHECK(keyrouter->hard_keys, return, "Invalid keyrouter resource.\n");

	for (i = 0; i < KEYROUTER_MAX_KEYS; i++) {
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.excl);
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.or_excl);
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.top);
		_keyrouter_list_free(&keyrouter->hard_keys[i].grab.shared);
		_keyrouter_list_free(&keyrouter->hard_keys[i].pressed);
	}

	free(keyrouter->hard_keys);
	keyrouter->hard_keys = NULL;
	free(keyrouter);
	keyrouter = NULL;
}
