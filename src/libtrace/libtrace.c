/*
 * Terrible hacks... I don't want *anything* to be exported, so
 * everything has to be in the same file.
 */

#include <sys/types.h>
#include <stdarg.h>

static void init_libtrace(void);
static size_t strlen(const char *);
static int strncmp(const char *, const char *, size_t);
static char * getenv(const char *);
static unsigned long atoi(const char *);
static char * bcopy(const char *, char *, int);
static int sprintf(char * buf, const char *fmt, ...);
static int strcmp(const char * cs,const char * ct);
static size_t strnlen(const char * s, size_t count);
static int vsprintf(char *buf, const char *fmt, va_list args);
static char * strcat(char * dest, const char * src);

#include "__setfpucw.c"
#include "init_libtrace.c"
#include "strlen.c"
#include "strncmp.c"
#include "getenv.c"
#include "atoi.c"
#include "bcopy.c"
#include "print_results.c"
#include "sprintf.c"
#include "vsprintf.c"
#include "strcmp.c"
#include "strnlen.c"
#include "strcat.c"
