#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "ltrace.h"
#include "options.h"
#include "defs.h"

FILE * output;
int opt_a = DEFAULT_ACOLUMN;	/* default alignment column for results */
int opt_d = 0;			/* debug */
int opt_i = 0;			/* instruction pointer */
int opt_s = DEFAULT_STRLEN;	/* default maximum # of bytes printed in strings */
int opt_S = 0;			/* display syscalls */
int opt_L = 1;			/* display library calls */
int opt_f = 0;			/* trace child processes as they are created */
char * opt_u = NULL;		/* username to run command as */
int opt_t = 0;			/* print absolute timestamp */

/* List of pids given to option -p: */
struct opt_p_t * opt_p = NULL;	/* attach to process with a given pid */

static void usage(void)
{
	fprintf(stderr, "Usage: ltrace [-dfiLSttt] [-a column] [-s strlen] [-o filename]\n"
			"              [-u username] [-p pid] ... [command [arg ...]]\n\n");
}

static char * search_for_command(char * filename)
{
	static char pathname[PATH_MAX];
	char *path;
	int m, n;

	if (strchr(filename, '/')) {
		return filename;
	}
	for (path = getenv("PATH"); path && *path; path += m) {
		if (strchr(path, ':')) {
			n = strchr(path, ':') - path;
			m = n + 1;
		} else {
			m = n = strlen(path);
		}
		strncpy(pathname, path, n);
		if (n && pathname[n - 1] != '/') {
			pathname[n++] = '/';
		}
		strcpy(pathname + n, filename);
		if (!access(pathname, X_OK)) {
			return pathname;
		}
	}
	return filename;
}

char ** process_options(int argc, char **argv)
{
	char *nextchar = NULL;

	output = stderr;

	while(1) {
		if (!nextchar || !(*nextchar)) {
			if (!argv[1] || argv[1][0] != '-' || !argv[1][1]) {
				break;
			}
			nextchar = &argv[1][1];
			argc--; argv++;
		}
		switch (*nextchar++) {
			case 'd':	opt_d++;
					break;
			case 'f':	opt_f = 1;
					break;
			case 'i':	opt_i++;
					break;
			case 'L':	opt_L = 0;
					break;
			case 'S':	opt_S = 1;
					break;
			case 't':	opt_t++;
					break;
			case 'a':	if (!argv[1]) { usage(); exit(1); }
					opt_a = atoi(argv[1]);
					argc--; argv++;
					break;
			case 's':	if (!argv[1]) { usage(); exit(1); }
					opt_s = atoi(argv[1]);
					argc--; argv++;
					break;
			case 'o':	if (!argv[1]) { usage(); exit(1); }
					output = fopen(argv[1], "w");
					if (!output) {
						fprintf(stderr, "Can't open %s for output: %s\n", argv[1], strerror(errno));
						exit(1);
					}
					argc--; argv++;
					break;
			case 'u':	if (!argv[1]) { usage(); exit(1); }
					opt_u = argv[1];
					argc--; argv++;
					break;
			case 'p':	if (!argv[1]) { usage(); exit(1); }
				{
					struct opt_p_t * tmp = malloc(sizeof(struct opt_p_t));
					if (!tmp) {
						perror("malloc");
						exit(1);
					}
					tmp->pid = atoi(argv[1]);
					tmp->next = opt_p;
					opt_p = tmp;
					argc--; argv++;
					break;
				}
			default:	fprintf(stderr, "Unknown option '%c'\n", *(nextchar-1));
					usage();
					exit(1);
		}
	}

	if (!opt_p && argc<2) {
		usage();
		exit(1);
	}
	if (argc>1) {
		command = search_for_command(argv[1]);
	}
	return &argv[1];
}
