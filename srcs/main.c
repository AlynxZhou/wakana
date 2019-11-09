#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "server.h"
#include "output.h"
#include <wlr/types/wlr_matrix.h>

int main(int argc, char *argv[])
{
	struct wkn_server *server = wkn_server_create();

	const char *socket = wl_display_add_socket_auto(server->wl_display);
	assert(socket);

	if (!wlr_backend_start(server->wlr_backend)) {
		fprintf(stderr, "Failed to start wlr backend!\n");
		wkn_server_destroy(server);
		exit(1);
	}

	setenv("WAYLAND_DISPLAY", socket, true);
	printf("Running Wakana on WAYLAND_DISPLAY=%s\n", socket);

	wl_display_init_shm(server->wl_display);

	wl_display_run(server->wl_display);

	wkn_server_destroy(server);
	return 0;
}
