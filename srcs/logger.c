#include <stdio.h>
#include "logger.h"

int wkn_logger_log(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int res = vfprintf(stdout, fmt, args);
	va_end(args);
	return res;
}

int wkn_logger_debug(const char *fmt, ...)
{
#ifdef __DEBUG__
	va_list args;
	va_start(args, fmt);
	int res = vfprintf(stdout, fmt, args);
	va_end(args);
	return res;
#else
	return 0;
#endif
}

int wkn_logger_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int res = vfprintf(stderr, fmt, args);
	va_end(args);
	return res;
}
