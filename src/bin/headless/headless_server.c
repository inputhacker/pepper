#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <pepper.h>
#include <headless_server.h>

static int
handle_sigint(int signal_number, void *data)
{
	struct wl_display *display = (struct wl_display *)data;
	wl_display_terminate(display);

	return 0;
}

static pepper_bool_t
init_signal(pepper_compositor_t *compositor)
{
	struct wl_display *display;
	struct wl_event_loop *loop;
	struct wl_event_source *sigint;

	display = pepper_compositor_get_display(compositor);
	loop = wl_display_get_event_loop(display);
	sigint = wl_event_loop_add_signal(loop, SIGINT, handle_sigint, display);
	if (!sigint)
		return PEPPER_FALSE;

	return PEPPER_TRUE;
}

int main(int argc, char *argv[])
{
	const char *socket_name = NULL;
	pepper_compositor_t *compositor = NULL;
	pepper_bool_t ret;

	/* set STDOUT/STDERR bufferless */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "wayland-0";

	if (!getenv("XDG_RUNTIME_DIR"))
		setenv("XDG_RUNTIME_DIR", "/run", 1);

	/* create pepper compositir */
	compositor = pepper_compositor_create(socket_name);
	PEPPER_CHECK(compositor, return EXIT_FAILURE, "Failed to create compositor !");

	/* Init input for headless */
	ret = headless_input_init(compositor);
	PEPPER_CHECK(ret, goto end, "headless_input_init() failed\n");

	/* Init Output */
	ret = headless_output_init(compositor);
	PEPPER_CHECK(ret, goto end, "headless_output_init() failed.\n");

	/* Init Shell */
	ret = headless_shell_init(compositor);
	PEPPER_CHECK(ret, goto end, "headless_shell_init() failed.\n");

	/* Init Signal for SIGINT */
	init_signal(compositor);

	/* run event loop */
	wl_display_run(pepper_compositor_get_display(compositor));

end:
	/* Deinit Process */
	headless_shell_deinit(compositor);
	headless_input_deinit(compositor);
	headless_output_deinit(compositor);
	pepper_compositor_destroy(compositor);

	return EXIT_SUCCESS;
}
