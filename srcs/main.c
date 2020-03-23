#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wlr/util/log.h>
#include "logger.h"
#include "server.h"
#include "output.h"

int wkn_drop_permission(void)
{
        if (getuid() != geteuid() || getgid() != getegid()) {
                // Set the gid and uid in the correct order.
                if (setgid(getgid()) != 0) {
                        wkn_logger_error(
				"Failed to drop root group, "
				"refusing to start.\n"
			);
                        return -1;
                }
                if (setuid(getuid()) != 0) {
                        wkn_logger_error(
				"Failed to drop root user, "
				"refusing to start.\n"
			);
                        return -2;
                }
        }
        if (setgid(0) != -1 || setuid(0) != -1) {
                wkn_logger_error(
			"Unable to drop root (we shouldn't be able to "
                        "restore it after setuid), refusing to start.\n"
		);
                return -3;
        }
        return 0;
}

int main(int argc, char *argv[])
{
	wlr_log_init(WLR_DEBUG, NULL);
	struct wkn_server *server = wkn_server_create();

	// We need permission to create backends.
	// And we can drop permission before creating global.
	if (wkn_drop_permission() < 0)
		exit(EXIT_FAILURE);

	wkn_server_setup(server);

	const char *socket = wl_display_add_socket_auto(server->wl_display);
	assert(socket);

	if (!wlr_backend_start(server->wlr_backend)) {
		wkn_logger_error("Failed to start backend!\n");
		wkn_server_destroy(server);
		exit(EXIT_FAILURE);
	}

	wl_display_init_shm(server->wl_display);

	setenv("WAYLAND_DISPLAY", socket, true);
	wkn_logger_log("Running Wakana on WAYLAND_DISPLAY=%s\n", socket);

	wl_display_run(server->wl_display);

	wkn_server_destroy(server);
	return 0;
}
