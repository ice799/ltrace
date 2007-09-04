#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "options.h"
#include "output.h"

void
debug_(int level, const char *file, int line, const char *func,
		const char *fmt, ...)
{
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

// The following section provides a way to print things, like hex dumps,
// with out using buffered output.  This was written by Steve Munroe of IBM.

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ptrace.h>

static int xwritehexl(long i)
{
	int rc = 0;
	char text[17];
	int j;
	unsigned long temp = (unsigned long)i;

	for (j = 15; j >= 0; j--) {
		char c;
		c = (char)((temp & 0x0f) + '0');
		if (c > '9') {
			c = (char)(c + ('a' - '9' - 1));
		}
		text[j] = c;
		temp = temp >> 4;
	}

	rc = write(1, text, 16);
	return rc;
}

static int xwritec(char c)
{
	char temp = c;
	char *text = &temp;
	int rc = 0;
	rc = write(1, text, 1);
	return rc;
}

static int xwritecr(void)
{
	return xwritec('\n');
}

static int xwritedump(void *ptr, long addr, int len)
{
	int rc = 0;
	long *tprt = (long *)ptr;
	int i;

	for (i = 0; i < len; i += 8) {
		xwritehexl(addr);
		xwritec('-');
		xwritec('>');
		xwritehexl(*tprt++);
		xwritecr();
		addr += sizeof(long);
	}

	return rc;
}

int xinfdump(long pid, void *ptr, int len)
{
	int rc;
	int i;
	long wrdcnt = len / sizeof(long) + 1;
	long *infwords = malloc(wrdcnt * sizeof(long));
	long addr = (long)ptr;

	addr = ((addr + sizeof(long) - 1) / sizeof(long)) * sizeof(long);

	for (i = 0; i < wrdcnt; ++i) {
		infwords[i] = ptrace(PTRACE_PEEKTEXT, pid, addr);
		addr += sizeof(long);
	}

	rc = xwritedump(infwords, (long)ptr, len);

	free(infwords);
	return rc;
}
