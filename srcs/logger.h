#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "server.h"

struct wkn_logger {
	struct wkn_server *server;
	bool debug;
	char *logpath;
	FILE *logfile;
};

struct wkn_logger *wkn_logger_create(
	struct wkn_server *server,
	bool debug,
	char *logpath
);
int wkn_logger_log(struct wkn_logger *logger, const char *fmt, ...);
int wkn_logger_debug(struct wkn_logger *logger, const char *fmt, ...);
int wkn_logger_error(struct wkn_logger *logger, const char *fmt, ...);
void wkn_logger_destroy(struct wkn_logger *logger);

#endif
