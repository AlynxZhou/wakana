#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logger.h"
#include "server.h"
#include "output.h"

int main(int argc, char *argv[])
{
	struct wkn_server *server = wkn_server_create();

	const char *socket = wl_display_add_socket_auto(server->wl_display);
	assert(socket);

	if (!wlr_backend_start(server->wlr_backend)) {
		wkn_logger_error("Failed to start wlr backend!\n");
		wkn_server_destroy(server);
		exit(1);
	}

	wl_display_init_shm(server->wl_display);

	setenv("WAYLAND_DISPLAY", socket, true);
	wkn_logger_log("Running Wakana on WAYLAND_DISPLAY=%s\n", socket);

	wl_display_run(server->wl_display);

	wkn_server_destroy(server);
	return 0;
}
