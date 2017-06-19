#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <wayland-client.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <tizen-extension-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#define DEBUG
#ifdef DEBUG
#define TRACE(fmt, ...)	\
	do { \
		printf("[sample-client : %s] "fmt, __FUNCTION__, ##__VA_ARGS__); \
	} while (0)
#else
#define TRACE(fmt, ...)
	do { \
		;
	} while (0)
#endif

#define ERROR_CHECK(exp, action, fmt, ...) \
	do { \
		if (!(exp)) \
		{ \
			printf(fmt, ##__VA_ARGS__);	\
			action; \
		} \
	} while (0)

struct sample_client
{
	struct wl_display *display;
	struct wl_compositor *compositor;
	struct wl_registry *registry;
	struct tizen_keyrouter *keyrouter;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct wl_pointer *pointer;
	struct wl_touch *touch;

	struct xdg_shell *shell;
	struct wl_shm *shm;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct wl_buffer *buffer;

	struct xkb_context *xkb_context;
	struct xkb_keymap *keymap;

	pid_t pid;
	void *shm_data;
};

struct keyrouter_grab_list {
	int key;
	int mode;
	int err;
};

struct keycode_map
{
	xkb_keysym_t keysym;
	xkb_keycode_t *keycodes;
	int nkeycodes;
};

typedef struct keyrouter_grab_list keyrouter_grab_list_t;
typedef struct sample_client sample_client_t;
typedef struct keycode_map keycode_map_t;

const int width = 640;
const int height = 480;

static void
find_keycode(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
	keycode_map_t *found_keycodes = (keycode_map_t *)data;
	xkb_keysym_t keysym = found_keycodes->keysym;
	int nsyms = 0;
	const xkb_keysym_t *syms_out = NULL;

	ERROR_CHECK(keymap, return, "[%s] Invalid keymap !\n", __FUNCTION__);

	nsyms = xkb_keymap_key_get_syms_by_level(keymap, key, 0, 0, &syms_out);

	if (nsyms && syms_out)
	{
		if (*syms_out == keysym)
		{
			found_keycodes->nkeycodes++;
			found_keycodes->keycodes = realloc(found_keycodes->keycodes, sizeof(int)*found_keycodes->nkeycodes);
			found_keycodes->keycodes[found_keycodes->nkeycodes-1] = key;
		}
	}
}

int
xkb_keycode_from_keysym(struct xkb_keymap *keymap, xkb_keysym_t keysym, xkb_keycode_t **keycodes)
{
	keycode_map_t found_keycodes = {0,};
	found_keycodes.keysym = keysym;

	ERROR_CHECK(keymap, return 0, "[%s] Invalid keymap !\n", __FUNCTION__);

	xkb_keymap_key_for_each(keymap, find_keycode, &found_keycodes);

	*keycodes = found_keycodes.keycodes;
	return found_keycodes.nkeycodes;
}

int
xkb_init(sample_client_t *client)
{
	if (!client)
		return 0;

	client->xkb_context = xkb_context_new(0);
	ERROR_CHECK(client->xkb_context, return 0, "Failed to get xkb_context\n");

	return 1;
}

void
xkb_shutdown(sample_client_t *client)
{
	if (!client || !client->xkb_context)
		return;

	xkb_context_unref(client->xkb_context);
}

static int
set_cloexec_or_close(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);

	if (flags == -1)
		goto err;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto err;

	return fd;

err:
	close(fd);
	return -1;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
	int fd;

#ifdef HAVE_MKOSTEMP
	fd = mkostemp(tmpname, O_CLOEXEC);

	if (fd >= 0)
		unlink(tmpname);
#else
	fd = mkstemp(tmpname);

	if (fd >= 0)
	{
		fd = set_cloexec_or_close(fd);
		unlink(tmpname);
	}
#endif

	return fd;
}

int
os_create_anonymous_file(off_t size)
{
	static const char template[] = "/pepper-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;
	int ret;

	path = getenv("TIZEN_WAYLAND_SHM_DIR");

	if (!path)
		path = getenv("XDG_RUNTIME_DIR");

	if (!path)
	{
		setenv("XDG_RUNTIME_DIR", "/run", 1);
		path = getenv("XDG_RUNTIME_DIR");
	}

	if (!path) {
		errno = ENOENT;
		return -1;
	}

	name = malloc(strlen(path) + sizeof(template));
	if (!name)
		return -1;

	strncpy(name, path, strlen(path) + 1);
	strncat(name, template, sizeof(template));

	fd = create_tmpfile_cloexec(name);

	free(name);

	if (fd < 0)
		return -1;

#ifdef HAVE_POSIX_FALLOCATE
	ret = posix_fallocate(fd, 0, size);
	if (ret != 0) {
		close(fd);
		errno = ret;
		return -1;
	}
#else
	ret = ftruncate(fd, size);
	if (ret < 0) {
		close(fd);
		return -1;
	}
#endif

	return fd;
}

static void
paint_pixels(sample_client_t *client)
{
	int n;
	uint32_t *pixel = client->shm_data;

	for (n =0; n < width*height; n++)
	{
		*pixel++ = 0xffff;
	}
}

static struct wl_buffer *
create_buffer(sample_client_t *client)
{
	struct wl_shm_pool *pool;
	int stride = width * 4; // 4 bytes per pixel
	int size = stride * height;
	int fd;
	struct wl_buffer *buff;

	fd = os_create_anonymous_file(size);

	if (fd < 0)
	{
		TRACE("... creating a buffer file has been failed: %m\n");
		exit(1);
	}

	client->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (client->shm_data == MAP_FAILED)
	{
		TRACE("... mmap failed: %m\n");
		close(fd);
		exit(1);
	}

	pool = wl_shm_create_pool(client->shm, fd, size);
	buff = wl_shm_pool_create_buffer(pool, 0,	width, height, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);

	return buff;
}

static void
create_window(sample_client_t *client)
{
	client->buffer = create_buffer(client);

	wl_surface_attach(client->surface, client->buffer, 0, 0);
	wl_surface_damage(client->surface, 0, 0, width, height);
	wl_surface_commit(client->surface);
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
	TRACE("... SHM Format %d\n", format);
}

struct wl_shm_listener shm_listener = {
	shm_format
};

static void
xdg_shell_ping(void *data, struct xdg_shell *shell, uint32_t serial)
{
	(void) data;
	xdg_shell_pong(shell, serial);
}

static const struct xdg_shell_listener xdg_shell_listener =
{
	xdg_shell_ping,
};


static void
keygrab_request(struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t key, uint32_t mode)
{
   tizen_keyrouter_set_keygrab(tizen_keyrouter, surface, key, mode);

   TRACE("... request set_keygrab (key:%d, mode:%d)!\n",  key, mode);
}

static void pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
   (void) data;
   (void) wl_pointer;
   (void) serial;
   (void) surface;
   (void) surface_x;
   (void) surface_y;

   TRACE("... serial=%d, x=%.f, y=%.f\n", serial, wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y));
}

static void pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface)
{
   (void) data;
   (void) wl_pointer;
   (void) serial;
   (void) surface;

   TRACE("... serial=%d\n", serial);
}

static void pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
   (void) data;
   (void) wl_pointer;
   (void) time;
   (void) surface_x;
   (void) surface_y;

   TRACE("... time=%d, x=%.f, y=%.f\n", time, wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y));
}

static void pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
   (void) data;
   (void) wl_pointer;
   (void) serial;
   (void) time;
   (void) button;
   (void) state;

   TRACE("... serial=%d, time=%d, button=%d, state=%d\n", serial, time, button, state);
}

static void pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
   (void) data;
   (void) wl_pointer;
   (void) time;
   (void) axis;
   (void) value;

   TRACE("... time=%d, axis=%d, value=%.f\n", time, axis, wl_fixed_to_double(value));
}

static void
keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
	(void) data;
	(void) keyboard;
	(void) format;
	(void) fd;
	(void) size;

	sample_client_t *client = (sample_client_t *)data;
	char *map = NULL;

	TRACE("...WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1=%d, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP=%d\n",
	   				WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP);
	TRACE("... format=%d, fd=%d, size=%d\n",  format, fd, size);

	if (!client->xkb_context)
	{
		TRACE("... This client failed to make xkb context\n");
		close(fd);
		return;
	}

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		TRACE("... Invaild format: %d\n", format);
		close(fd);
		return;
	}

	map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

	if (map == MAP_FAILED)
	{
		TRACE("... Failed to mmap from fd(%d) size(%d)\n", fd, size);
		close(fd);
		return;
	}

	client->keymap = xkb_map_new_from_string(client->xkb_context, map,
	                       XKB_KEYMAP_FORMAT_TEXT_V1, 0);
	if (client->keymap)
		TRACE("... Failed to get keymap from fd(%d)\n", fd);

	munmap(map, size);
	close(fd);
}

static void
keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) surface;
	(void) keys;

	TRACE("... serial=%d\n",  serial);
}

static void
keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) surface;

	TRACE("... serial=%d\n",  serial);
}

static void
keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) time;
	(void) key;
	(void) state;

	TRACE("... serial=%d, time=%d, key=%d, state=%d\n", serial, time, key, state);
}

static void
keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	(void) data;
	(void) keyboard;
	(void) serial;
	(void) mods_depressed;
	(void) mods_latched;
	(void) mods_locked;
	(void) group;

	TRACE("... serial=%d, mods_depressed=%d, mods_latched=%d, mods_locked=%d, group=%d\n", serial, mods_depressed, mods_latched, mods_locked, group);
}

static void
touch_down(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	(void) data;
	(void) wl_touch;
	(void) serial;
	(void) time;
	(void) surface;
	(void) id;
	(void) x_w;
	(void) y_w;

	TRACE("... serial=%d, time=%d, id=%d, x_w=%.f, y_w=%.f\n", serial, time, id, wl_fixed_to_double(x_w), wl_fixed_to_double(y_w));
}

static void
touch_up(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, int32_t id)
{
	(void) data;
	(void) wl_touch;
	(void) serial;
	(void) time;
	(void) id;

	TRACE("... serial=%d, time=%d, id=%d\n",  serial, time, id);
}

static void
touch_motion(void *data, struct wl_touch *wl_touch, uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	(void) data;
	(void) wl_touch;
	(void) time;
	(void) id;
	(void) x_w;
	(void) y_w;

	TRACE("... time=%d, id=%d, x_w=%.f, y_w=%.f\n", time, id, wl_fixed_to_double(x_w), wl_fixed_to_double(y_w));
}

static void
touch_frame(void *data, struct wl_touch *wl_touch)
{
	(void) data;
	(void) wl_touch;

	TRACE("...\n");
}

static void
touch_cancel(void *data, struct wl_touch *wl_touch)
{
	(void) data;
	(void) wl_touch;

	TRACE("...\n");
}

/* Define pointer event handlers */
static const struct wl_pointer_listener pointer_listener = {
	.enter = pointer_enter,
	.leave = pointer_leave,
	.motion = pointer_motion,
	.button = pointer_button,
	.axis = pointer_axis
};

/* Define touch event handlers */
static const struct wl_touch_listener touch_listener = {
	.down = touch_down,
	.up = touch_up,
	.motion = touch_motion,
	.frame = touch_frame,
	.cancel = touch_cancel
};

/* Define keyboard event handlers */
static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers
};

static void
global_registry_add(void * data, struct wl_registry * registry, uint32_t id, const char * interface, uint32_t version)
{
	sample_client_t *client = (sample_client_t *)data;

	if (0 == strncmp(interface, "wl_compositor", 13))
	{
		client->compositor = wl_registry_bind(client->registry, id, &wl_compositor_interface, 1);
		if (client->compositor) TRACE("[PID:%d] Succeed to bind wl_compositor_interface !\n", client->pid);
	}
	else if (0 == strncmp(interface, "tizen_keyrouter", 12))
	{
		client->keyrouter = wl_registry_bind(client->registry, id, &tizen_keyrouter_interface, 1);
		if (client->keyrouter) TRACE("[PID:%d] Succeed to bind tizen_keyrouter_interface !\n", client->pid);
	}
	else if (0 == strncmp(interface, "xdg_shell", 9))
	{
		client->shell = wl_registry_bind(client->registry, id, &xdg_shell_interface, 1);

		if (client->shell)
			TRACE("[PID:%d] Succeed to bind xdg_shell interface !\n", client->pid);

		xdg_shell_use_unstable_version(client->shell, 5);
		xdg_shell_add_listener(client->shell, &xdg_shell_listener, client->display);
	}
	else if (0 == strncmp(interface, "wl_shm", 6))
	{
		client->shm = wl_registry_bind(client->registry, id, &wl_shm_interface, 1);
		wl_shm_add_listener(client->shm, &shm_listener, NULL);

		if (client->shm)
			TRACE("[PID:%d] Succeed to bind wl_shm_interface !\n", client->pid);
	}
	else if (0 == strncmp(interface, "wl_seat", 7))
	{
		client->seat = wl_registry_bind(client->registry, id, &wl_seat_interface, 1);

		if (client->seat)
			TRACE("[PID:%d] Succeed to bind wl_seat_interface !\n", client->pid);

		client->pointer = wl_seat_get_pointer(client->seat);
		wl_pointer_add_listener(client->pointer, &pointer_listener, client);

		client->keyboard = wl_seat_get_keyboard(client->seat);
		wl_keyboard_add_listener(client->keyboard, &keyboard_listener, client);

		client->touch = wl_seat_get_touch(client->seat);
		wl_touch_add_listener(client->touch, &touch_listener, client);
	}
	else
	{
		TRACE("[PID:%d] An unhandled global object's interface : %s\n", client->pid, interface ? interface : "NULL");
	}
}

static void
global_registry_remove(void * data, struct wl_registry * registry, uint32_t id)
{
	sample_client_t *client = (sample_client_t *)data;

	TRACE("[PID:%d] global object (id:0x%x) has been removed !\n", client->pid, id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_add,
    global_registry_remove
};

static void
keygrab_notify(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, uint32_t key, uint32_t mode, uint32_t error)
{
	TRACE("... key=%d, mode=%d, error=%d\n",  key, mode, error);
}

static void
keygrab_notify_list(void *data, struct tizen_keyrouter *tizen_keyrouter, struct wl_surface *surface, struct wl_array *grab_list)
{
	TRACE("... list\n");
}


static struct tizen_keyrouter_listener keyrouter_listener = {
	keygrab_notify,
	keygrab_notify_list
};

static void
do_keygrab(sample_client_t *client, const char *keyname, uint32_t mode)
{
	xkb_keysym_t keysym = 0x0;
	int nkeycodes=0;
	xkb_keycode_t *keycodes = NULL;
	int i;

	keysym = xkb_keysym_from_name(keyname, XKB_KEYSYM_NO_FLAGS);
	nkeycodes = xkb_keycode_from_keysym(client->keymap, keysym, &keycodes);

	for (i=0; i<nkeycodes; i++)
	{
		TRACE("%s's keycode is %d (nkeycode: %d)\n", keyname, keycodes[i], nkeycodes);
		keygrab_request(client->keyrouter, client->surface, keycodes[i], mode);
	}

	free(keycodes);
	keycodes = NULL;
}

int main(int argc, char **argv)
{
	int res;
	sample_client_t *client = NULL;

	const char *use_xdg_shell = NULL;
	const char *use_keyrouter = NULL;

	use_xdg_shell = getenv("USE_XDG_SHELL");
	use_keyrouter = getenv("USE_KEYROUTER");

	if (!use_xdg_shell)
		TRACE("* Note : XDG SHELL can be initialized by setting USE_XDG_SHELL environment variable !\n");
	if (!use_keyrouter)
		TRACE("* Note : tizen_keyrouter interface can be initialized by setting USE_KEYROUTER environment variable !\n");

	client = calloc(1, sizeof(sample_client_t));
	ERROR_CHECK(client, goto shutdown, "Failed to allocate memory for sample client !\n");

	client->pid = getpid();
	client->display = wl_display_connect(NULL);
	ERROR_CHECK(client->display, goto shutdown, "[PID:%d] Failed to connect to wayland server !\n", client->pid);

	res = xkb_init(client);
	ERROR_CHECK(res, goto shutdown, "Failed to init xkb !\n");

	client->registry = wl_display_get_registry(client->display);
	ERROR_CHECK(client->registry, goto shutdown, "[PID:%d] Failed to get registry !\n", client->pid);

	wl_registry_add_listener(client->registry, &registry_listener, client);

	assert(wl_display_dispatch(client->display) != -1);
	assert(wl_display_roundtrip(client->display) != -1);

	ERROR_CHECK(client->compositor, goto shutdown, "[PID:%d] Failed to bind to the compositor interface !\n", client->pid);

	if (use_keyrouter)
		ERROR_CHECK(client->keyrouter, goto shutdown, "[PID:%d] Failed to bind to the keyrouter interface !\n", client->pid);

	if (use_xdg_shell)
		ERROR_CHECK(client->shell, goto shutdown, "[PID:%d] Failed to bind to the xdg shell interface !\n", client->pid);

	client->surface = wl_compositor_create_surface(client->compositor);
	ERROR_CHECK(client->surface, goto shutdown, "[PID:%d] can't create surface\n", client->pid);

	if (use_xdg_shell)
	{
		client->xdg_surface = xdg_shell_get_xdg_surface(client->shell, client->surface);
		ERROR_CHECK(client->xdg_surface, goto shutdown, "[PID:%d] can't create shell surface\n", client->pid);

		xdg_surface_set_title(client->xdg_surface, "sample client");
	}

	create_window(client);
	paint_pixels(client);

	if (use_keyrouter)
	{
		if (0 > tizen_keyrouter_add_listener(client->keyrouter, &keyrouter_listener, NULL))
		{
			TRACE("[PID:%d] Failed on tizen_keyrouter_add_listener !\n", client->pid);
			return 0;
		}

		do_keygrab(client, "XF86Home", TIZEN_KEYROUTER_MODE_OVERRIDABLE_EXCLUSIVE);
	}

	while (wl_display_dispatch(client->display) != -1)
		;

	wl_display_disconnect(client->display);
	client->display = NULL;

	return 0;

shutdown:

	if(!client)
		return 0;

	xkb_shutdown(client);

	return 0;
}

