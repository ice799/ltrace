#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_LIBIBERTY

#include <string.h>

#include "demangle.h"

char * my_demangle(char * function_name)
{
	char * tmp;

	tmp = cplus_demangle(function_name+strspn(function_name,"_"),DMGL_ANSI | DMGL_PARAMS);
	return tmp ? tmp : strdup(function_name);
}

#endif
