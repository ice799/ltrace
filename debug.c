#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "options.h"
#include "output.h"

void
debug_(int level, char *file, int line, char *func, char *fmt, ...) {
	char buf[1024];
	va_list args;

	if (opt_d < level) {
		return;
	}
	va_start(args, fmt);
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	output_line(NULL, "DEBUG: %s:%d: %s(): %s", file, line, func, buf);
}
