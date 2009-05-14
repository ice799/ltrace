#include <features.h>

/* debug levels:
 */
enum {
	DEBUG_EVENT    = 0x10,
	DEBUG_PROCESS  = 0x20,
	DEBUG_FUNCTION = 0x40
};

void debug_(int level, const char *file, int line,
		const char *fmt, ...) __attribute__((format(printf,4,5)));

int xinfdump(long, void *, int);

# define debug(level, expr...) debug_(level, __FILE__, __LINE__, expr)

