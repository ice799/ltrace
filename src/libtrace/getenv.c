#include <sys/types.h>

extern char **__environ;

static char * getenv(const char *name)
{
	register const size_t len = strlen(name);
	register char **ep;

	for (ep = __environ; *ep != NULL; ++ep)
		if (!strncmp(*ep, name, len) && (*ep)[len] == '=')
			return(&(*ep)[len + 1]);

	return(NULL);
}
