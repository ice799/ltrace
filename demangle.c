#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_LIBIBERTY

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "options.h"
#include "output.h"
#include "demangle.h"

#include "dict.h"

/*****************************************************************************/

static struct dict * d = NULL;

static void
my_demangle_dict_clear(void) {
	/* FIXME TODO XXX: I should also free all (key,value) pairs */
	dict_clear(d);
}

const char *
my_demangle(const char * function_name) {
	const char * tmp, * fn_copy;

	if (!d) {
		d = dict_init(dict_key2hash_string, dict_key_cmp_string);
		atexit(my_demangle_dict_clear);
	}
 
	tmp = dict_find_entry(d, (void *)function_name);
	if (!tmp) {
		fn_copy = strdup(function_name);
		tmp = cplus_demangle(function_name+strspn(function_name, "_"), DMGL_ANSI | DMGL_PARAMS);
		if (!tmp) tmp = fn_copy;
		if (tmp) dict_enter(d, (void *)fn_copy, (void *)tmp);
	}
	return tmp;
}

#endif
