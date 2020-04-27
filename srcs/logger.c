#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"

struct wkn_logger *wkn_logger_create(
	struct wkn_server *server,
	bool debug,
	char *logpath
)
{
	struct wkn_logger *logger = malloc(sizeof(*logger));
	assert(logger);
	logger->server = server;
#ifdef __DEBUG__
	logger->debug = debug;
#endif
	logger->logpath = NULL;
	logger->logfile = NULL;

	// Print to stdout and stderr if no log file provided.
	if (logpath) {
		logger->logfile = fopen(logpath, "a");
		if (logger->logfile) {
			logger->logpath = strdup(logpath);
		} else {
			fprintf(
				stderr,
				"%s:%d: Failed to open %s, "
				"using stdout and stderr.\n",
				__FILE__,
				__LINE__,
				logpath
			);
		}
	}

	return logger;
}

int wkn_logger_log(struct wkn_logger *logger, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	FILE *logfile = logger->logpath ? logger->logfile : stdout;
	int res = vfprintf(logfile, fmt, args);
	va_end(args);
	return res;
}

int wkn_logger_debug(struct wkn_logger *logger, const char *fmt, ...)
{
#ifdef __DEBUG__
	if (!logger->debug)
		return 0;
	va_list args;
	va_start(args, fmt);
	FILE *logfile = logger->logpath ? logger->logfile : stdout;
	int res = vfprintf(logfile, fmt, args);
	va_end(args);
	return res;
#else
	return 0;
#endif
}

int wkn_logger_error(struct wkn_logger *logger, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	FILE *logfile = logger->logpath ? logger->logfile : stderr;
	int res = vfprintf(logfile, fmt, args);
	va_end(args);
	return res;
}

void wkn_logger_destroy(struct wkn_logger *logger)
{
	if (!logger)
		return;
	if (logger->logfile)
		fclose(logger->logfile);
	if (logger->logpath)
		free(logger->logpath);
	free(logger);
}
