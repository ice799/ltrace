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

/*****************************************************************************/

/*
  The string dictionary code done by Morten Eriksen <mortene@sim.no>.

  FIXME: since this is a generic string dictionary, it should perhaps
  be cleaned up a bit, "object-ified" and placed in its own .c + .h
  pair of files? 19990702 mortene.
*/

struct dict_entry
{
	unsigned int key;
	const char * mangled, * demangled;
	struct dict_entry * next;
};

#define DICTTABLESIZE 997 /* Semi-randomly selected prime number. */
static struct dict_entry * dict_buckets[DICTTABLESIZE];
static int dict_initialized = 0;

static void dict_init(void);
static void dict_clear(void);
static void dict_enter(const char * mangled, const char * demangled);
static const char * dict_find_entry(const char * mangled);
static unsigned int dict_hash_string(const char * s);


static void dict_init(void)
{
	int i;
	/* FIXME: is this necessary? Check with ANSI C spec. 19990702 mortene. */
	for (i = 0; i < DICTTABLESIZE; i++) dict_buckets[i] = NULL;
	dict_initialized = 1;
}

static void dict_clear(void)
{
	int i;
	struct dict_entry * entry, * nextentry;

	for (i = 0; i < DICTTABLESIZE; i++) {
		for (entry = dict_buckets[i]; entry != NULL; entry = nextentry) {
			nextentry = entry->next;
			free((void *)(entry->mangled));
			if (entry->mangled != entry->demangled)
				free((void *)(entry->demangled));
			free(entry);
		}
		dict_buckets[i] = NULL;
	}
}

static void dict_enter(const char * mangled, const char * demangled)
{
	struct dict_entry * entry, * newentry;
	unsigned int key = dict_hash_string(mangled);

	newentry = malloc(sizeof(struct dict_entry));
	if (!newentry) {
		perror("malloc");
		return;
	}

	newentry->key = key;
	newentry->mangled = mangled;
	newentry->demangled = demangled;
	newentry->next = NULL;

	entry = dict_buckets[key % DICTTABLESIZE];
	while (entry && entry->next) entry = entry->next;

	if (entry) entry->next = newentry;
	else dict_buckets[key % DICTTABLESIZE] = newentry;

	if (opt_d > 2)
		output_line(0, "new dict entry: '%s' -> '%s'\n", mangled, demangled);
}

static const char * dict_find_entry(const char * mangled)
{
	unsigned int key = dict_hash_string(mangled);
	struct dict_entry * entry = dict_buckets[key % DICTTABLESIZE];
	while (entry) {
		if ((entry->key == key) && (strcmp(entry->mangled, mangled) == 0)) break;
		entry = entry->next;
	}
	return entry ? entry->demangled : NULL;
}

static unsigned int dict_hash_string(const char * s)
{
	unsigned int total = 0, shift = 0;

	while (*s) {
		total = total ^ ((*s) << shift);
		shift += 5;
		if (shift > 24) shift -= 24;
		s++;
	}
	return total;
}

#undef DICTTABLESIZE

/*****************************************************************************/

const char * my_demangle(const char * function_name)
{
	const char * tmp, * fn_copy;

	if (!dict_initialized) {
		dict_init();
		atexit(dict_clear);
	}
 
	tmp = dict_find_entry(function_name);
	if (!tmp) {
		fn_copy = strdup(function_name);
		tmp = cplus_demangle(function_name+strspn(function_name, "_"), DMGL_ANSI | DMGL_PARAMS);
		if (!tmp) tmp = fn_copy;
		if (tmp) dict_enter(fn_copy, tmp);
	}
	return tmp;
}

#endif
