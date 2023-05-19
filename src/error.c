/* src/error.c */

#include "error.h"

#include <stdio.h>
#include <stdarg.h>

int error_code = 0;

void error_printf (const char *fmt, ...) {
	va_list va;
	
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}
