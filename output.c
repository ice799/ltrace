#include <stdarg.h>

#include "ltrace.h"
#include "process.h"

static int new_line=1;

void send_left(const char * fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (opt_i) {
		fprintf(output, "[%08x] ", instruction_pointer);
	}
	vfprintf(output, fmt, args);
	va_end(args);
	new_line=0;
}

void send_right(const char * fmt, ...)
{
	va_list args;

	if (new_line==0) {
		va_start(args, fmt);
		if (opt_i) {
			fprintf(output, "[%08x] ", instruction_pointer);
		}
		vfprintf(output, fmt, args);
		va_end(args);
	}
	new_line=1;
}

void send_line(const char * fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (opt_i) {
		fprintf(output, "[%08x] ", instruction_pointer);
	}
	vfprintf(output, fmt, args);
	va_end(args);
	new_line=1;
}
