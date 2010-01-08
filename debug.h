#ifndef _DEBUG_H
#define _DEBUG_H

#include <features.h>

/* debug levels:
 */
enum {
	DEBUG_EVENT    = 010,
	DEBUG_PROCESS  = 020,
	DEBUG_FUNCTION = 040
};

void debug_(int level, const char *file, int line,
		const char *fmt, ...) __attribute__((format(printf,4,5)));

int xinfdump(long, void *, int);

# define debug(level, expr...) debug_(level, __FILE__, __LINE__, expr)

#endif
