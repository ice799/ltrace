#include "config.h"

extern char *cplus_demangle(const char *mangled, int options);

const char *my_demangle(const char *function_name);

/* Options passed to cplus_demangle (in 2nd parameter). */

#define DMGL_NO_OPTS    0	/* For readability... */
#define DMGL_PARAMS     (1 << 0)	/* Include function args */
#define DMGL_ANSI       (1 << 1)	/* Include const, volatile, etc */
#define DMGL_JAVA       (1 << 2)	/* Demangle as Java rather than C++. */
