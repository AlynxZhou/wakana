#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>

int wkn_logger_log(const char *fmt, ...);
int wkn_logger_debug(const char *fmt, ...);
int wkn_logger_error(const char *fmt, ...);

#endif
