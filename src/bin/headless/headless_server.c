#include <pepper.h>

int main(int argc, char *argv[])
{
	const char *socket_name = NULL;
	pepper_compositor_t *compositor = NULL;

	socket_name = getenv("WAYLAND_DISPLAY");

	if (!socket_name)
		socket_name = "headless-0";

	/* create pepper compositir */
	compositor = pepper_compositor_create(socket_name);
	PEPPER_CHECK(compositor, return EXIT_FAILURE, "Failed to create compositor !");

	/* run event loop */
	wl_display_run(pepper_compositor_get_display(compositor));

	pepper_compositor_destroy(compositor);

	return EXIT_SUCCESS;
}
