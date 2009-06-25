#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "common.h"
#include "options.h"

#ifdef USE_DEMANGLE
#include "demangle.h"
#endif

static int num_entries = 0;
static struct entry_st {
	char *name;
	int count;
	struct timeval tv;
} *entries = NULL;

static int tot_count = 0;
static unsigned long int tot_usecs = 0;

static void fill_struct(void *key, void *value, void *data)
{
	struct opt_c_struct *st = (struct opt_c_struct *)value;

	entries = realloc(entries, (num_entries + 1) * sizeof(struct entry_st));
	if (!entries) {
		perror("realloc()");
		exit(1);
	}
	entries[num_entries].name = (char *)key;
	entries[num_entries].count = st->count;
	entries[num_entries].tv = st->tv;

	tot_count += st->count;
	tot_usecs += 1000000 * st->tv.tv_sec;
	tot_usecs += st->tv.tv_usec;

	num_entries++;
}

static int compar(const void *a, const void *b)
{
	struct entry_st *en1, *en2;

	en1 = (struct entry_st *)a;
	en2 = (struct entry_st *)b;

	if (en2->tv.tv_sec - en1->tv.tv_sec) {
		return (en2->tv.tv_sec - en1->tv.tv_sec);
	} else {
		return (en2->tv.tv_usec - en1->tv.tv_usec);
	}
}

void show_summary(void)
{
	int i;

	num_entries = 0;
	entries = NULL;

	dict_apply_to_all(dict_opt_c, fill_struct, NULL);

	qsort(entries, num_entries, sizeof(*entries), compar);

	fprintf(options.output, "%% time     seconds  usecs/call     calls      function\n");
	fprintf(options.output, "------ ----------- ----------- --------- --------------------\n");
	for (i = 0; i < num_entries; i++) {
		unsigned long long int c;
		unsigned long long int p;
		c = 1000000 * (int)entries[i].tv.tv_sec +
		    (int)entries[i].tv.tv_usec;
		p = 100000 * c / tot_usecs + 5;
		fprintf(options.output, "%3lu.%02lu %4d.%06d %11lu %9d %s\n",
		       (unsigned long int)(p / 1000),
		       (unsigned long int)((p / 10) % 100),
		       (int)entries[i].tv.tv_sec, (int)entries[i].tv.tv_usec,
		       (unsigned long int)(c / entries[i].count),
		       entries[i].count,
#ifdef USE_DEMANGLE
		       options.demangle ? my_demangle(entries[i].name) :
#endif
		       entries[i].name);
	}
	fprintf(options.output, "------ ----------- ----------- --------- --------------------\n");
	fprintf(options.output, "100.00 %4lu.%06lu             %9d total\n", tot_usecs / 1000000,
	       tot_usecs % 1000000, tot_count);
}
