#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <wlr/util/log.h>
#include "logger.h"
#include "server.h"
#include "output.h"

int wkn_drop_permission(void)
{
        if (getuid() != geteuid() || getgid() != getegid()) {
                // Set the gid and uid in the correct order.
                if (setgid(getgid()) != 0) {
                        fprintf(
				stderr,
				"%s:%d: Failed to drop root group, "
				"refusing to start.\n",
				__FILE__,
				__LINE__
			);
                        return -1;
                }
                if (setuid(getuid()) != 0) {
                        fprintf(
				stderr,
				"%s:%d: Failed to drop root user, "
				"refusing to start.\n",
				__FILE__,
				__LINE__
			);
                        return -2;
                }
        }
        if (setgid(0) != -1 || setuid(0) != -1) {
                fprintf(
			stderr,
			"%s: %d: Unable to drop root (we shouldn't be able to "
                        "restore it after setuid), refusing to start.\n",
			__FILE__,
			__LINE__
		);
                return -3;
        }
        return 0;
}

void wkn_print_help(const char *program_name)
{
	printf("Usage: %s [OPTION...] <value>\n", program_name);
	printf("Options:\n");
	printf("\t-h, --help\tDisplay this help.\n");
#ifdef __DEBUG__
	printf("\t-d, --debug\tPrint debug log.\n");
#endif
	printf("\t-l <logpath>, --logpath=<logpath>\tCustom log path.\n");
}

int main(int argc, char *argv[])
{
	struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
#ifdef __DEBUG__
		{"debug", no_argument, NULL, 'd'},
#endif
		{"logpath", required_argument, NULL, 'l'},
		{NULL, 0, NULL, 0}
	};

	bool debug = false;
	char *logpath = NULL;
	int opt = 0;
	int opt_index = 0;
	while ((opt = getopt_long(
		argc,
		argv,
		"hdl:",
		long_options,
		&opt_index
	)) != -1) {
		switch (opt) {
		case 'h':
			wkn_print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
#ifdef __DEBUG__
		case 'd':
			debug = true;
			break;
#endif
		case 'l':
			logpath = optarg;
			break;
		default:
			fprintf(
				stderr,
				"%s:%d: Invalid option `-%c`.\n",
				__FILE__,
				__LINE__,
				opt
			);
			exit(EXIT_FAILURE);
			break;
		}
	}

	struct wkn_server *server = wkn_server_create();

	// We need permission to create backends.
	// And we can drop permission before creating global.
	if (wkn_drop_permission() < 0)
		exit(EXIT_FAILURE);

	wkn_server_setup(server, debug, logpath);

	const char *socket = wl_display_add_socket_auto(server->wl_display);
	assert(socket);

	if (!wlr_backend_start(server->wlr_backend)) {
		wkn_logger_error(server->logger, "Failed to start backend!\n");
		wkn_server_destroy(server);
		exit(EXIT_FAILURE);
	}

	wl_display_init_shm(server->wl_display);

	setenv("WAYLAND_DISPLAY", socket, true);
	wkn_logger_log(
		server->logger,
		"Running Wakana on WAYLAND_DISPLAY=%s\n",
		socket
	);

	wl_display_run(server->wl_display);

	wkn_server_destroy(server);
	return 0;
}
