#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ltrace.h"
#include "options.h"
#include "defs.h"

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif

#define SYSTEM_CONFIG_FILE SYSCONFDIR "/ltrace.conf"
#define USER_CONFIG_FILE "~/.ltrace.conf"

struct options_t options = {
	.align    = DEFAULT_ALIGN,    /* alignment column for results */
	.user     = NULL,             /* username to run command as */
	.syscalls = 0,                /* display syscalls */
	.libcalls = 1,                /* display library calls */
#ifdef USE_DEMANGLE
	.demangle = 0,                /* Demangle low-level symbol names */
#endif
	.indent = 0,                  /* indent output according to program flow */
	.output = NULL,               /* output to a specific file */
	.summary = 0,                 /* Report a summary on program exit */
	.debug = 0,                   /* debug */
	.arraylen = DEFAULT_ARRAYLEN, /* maximum # array elements to print */
	.strlen = DEFAULT_STRLEN,     /* maximum # of bytes printed in strings */
	.follow = 0,                  /* trace child processes */
};

#define MAX_LIBRARY		30
char *library[MAX_LIBRARY];
int library_num = 0;
static char *progname;		/* Program name (`ltrace') */
int opt_i = 0;			/* instruction pointer */
int opt_r = 0;			/* print relative timestamp */
int opt_t = 0;			/* print absolute timestamp */
int opt_T = 0;			/* show the time spent inside each call */

/* List of pids given to option -p: */
struct opt_p_t *opt_p = NULL;	/* attach to process with a given pid */

/* List of function names given to option -e: */
struct opt_e_t *opt_e = NULL;
int opt_e_enable = 1;

/* List of global function names given to -x: */
struct opt_x_t *opt_x = NULL;

/* List of filenames give to option -F: */
struct opt_F_t *opt_F = NULL;	/* alternate configuration file(s) */

#ifdef PLT_REINITALISATION_BP
/* Set a break on the routine named here in order to re-initialize breakpoints
   after all the PLTs have been initialzed */
char *PLTs_initialized_by_here = PLT_REINITALISATION_BP;
#endif

static void
usage(void) {
#if !(HAVE_GETOPT || HAVE_GETOPT_LONG)
	fprintf(stdout, "Usage: %s [command [arg ...]]\n"
		"Trace library calls of a given program.\n\n", progname);
#else
	fprintf(stdout, "Usage: %s [option ...] [command [arg ...]]\n"
		"Trace library calls of a given program.\n\n"
# if HAVE_GETOPT_LONG
		"  -a, --align=COLUMN  align return values in a secific column.\n"
# else
		"  -a COLUMN           align return values in a secific column.\n"
# endif
		"  -A ARRAYLEN         maximum number of array elements to print.\n"
		"  -c                  count time and calls, and report a summary on exit.\n"
# ifdef USE_DEMANGLE
#  if HAVE_GETOPT_LONG
		"  -C, --demangle      decode low-level symbol names into user-level names.\n"
#  else
		"  -C                  decode low-level symbol names into user-level names.\n"
#  endif
# endif
# if HAVE_GETOPT_LONG
		"  -d, --debug         print debugging info.\n"
# else
		"  -d                  print debugging info.\n"
# endif
		"  -e expr             modify which events to trace.\n"
		"  -f                  trace children (fork() and clone()).\n"
# if HAVE_GETOPT_LONG
		"  -F, --config=FILE   load alternate configuration file (may be repeated).\n"
# else
		"  -F FILE             load alternate configuration file (may be repeated).\n"
# endif
# if HAVE_GETOPT_LONG
		"  -h, --help          display this help and exit.\n"
# else
		"  -h                  display this help and exit.\n"
# endif
		"  -i                  print instruction pointer at time of library call.\n"
#  if HAVE_GETOPT_LONG
		"  -l, --library=FILE  print library calls from this library only.\n"
#  else
		"  -l FILE             print library calls from this library only.\n"
#  endif
		"  -L                  do NOT display library calls.\n"
# if HAVE_GETOPT_LONG
		"  -n, --indent=NR     indent output by NR spaces for each call level nesting.\n"
# else
		"  -n NR               indent output by NR spaces for each call level nesting.\n"
# endif
# if HAVE_GETOPT_LONG
		"  -o, --output=FILE   write the trace output to that file.\n"
# else
		"  -o FILE             write the trace output to that file.\n"
# endif
		"  -p PID              attach to the process with the process ID pid.\n"
		"  -r                  print relative timestamps.\n"
		"  -s STRLEN           specify the maximum string size to print.\n"
		"  -S                  display system calls.\n"
		"  -t, -tt, -ttt       print absolute timestamps.\n"
		"  -T                  show the time spent inside each call.\n"
		"  -u USERNAME         run command with the userid, groupid of username.\n"
# if HAVE_GETOPT_LONG
		"  -V, --version       output version information and exit.\n"
# else
		"  -V                  output version information and exit.\n"
# endif
		"  -x NAME             treat the global NAME like a library subroutine.\n"
#ifdef PLT_REINITALISATION_BP
		"  -X NAME             same as -x; and PLT's will be initialized by here.\n"
#endif
		"\nReport bugs to ltrace-devel@lists.alioth.debian.org\n",
		progname);
#endif
}

static char *
search_for_command(char *filename) {
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
		if (n + strlen(filename) + 1 >= PATH_MAX) {
			fprintf(stderr, "Error: filename too long\n");
			exit(1);
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

static void
guess_cols(void) {
	struct winsize ws;
	char *c;

	options.align = DEFAULT_ALIGN;
	c = getenv("COLUMNS");
	if (c && *c) {
		char *endptr;
		int cols;
		cols = strtol(c, &endptr, 0);
		if (cols > 0 && !*endptr) {
			options.align = cols * 5 / 8;
		}
	} else if (ioctl(1, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0) {
		options.align = ws.ws_col * 5 / 8;
	} else if (ioctl(2, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0) {
		options.align = ws.ws_col * 5 / 8;
	}
}

char **
process_options(int argc, char **argv) {
	progname = argv[0];
	options.output = stderr;

	guess_cols();

#if HAVE_GETOPT || HAVE_GETOPT_LONG
	while (1) {
		int c;
#if HAVE_GETOPT_LONG
		int option_index = 0;
		static struct option long_options[] = {
			{"align", 1, 0, 'a'},
			{"config", 1, 0, 'F'},
			{"debug", 0, 0, 'd'},
# ifdef USE_DEMANGLE
			{"demangle", 0, 0, 'C'},
#endif
			{"indent", 1, 0, 'n'},
			{"help", 0, 0, 'h'},
			{"library", 1, 0, 'l'},
			{"output", 1, 0, 'o'},
			{"version", 0, 0, 'V'},
			{0, 0, 0, 0}
		};
		c = getopt_long(argc, argv, "+cdfhiLrStTV"
# ifdef USE_DEMANGLE
				"C"
# endif
				"a:A:e:F:l:n:o:p:s:u:x:X:", long_options,
				&option_index);
#else
		c = getopt(argc, argv, "+cdfhiLrStTV"
# ifdef USE_DEMANGLE
				"C"
# endif
				"a:A:e:F:l:n:o:p:s:u:x:X:");
#endif
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'a':
			options.align = atoi(optarg);
			break;
		case 'A':
			options.arraylen = atoi(optarg);
			break;
		case 'c':
			options.summary++;
			break;
#ifdef USE_DEMANGLE
		case 'C':
			options.demangle++;
			break;
#endif
		case 'd':
			options.debug++;
			break;
		case 'e':
			{
				char *str_e = strdup(optarg);
				if (!str_e) {
					perror("ltrace: strdup");
					exit(1);
				}
				if (str_e[0] == '!') {
					opt_e_enable = 0;
					str_e++;
				}
				while (*str_e) {
					struct opt_e_t *tmp;
					char *str2 = strchr(str_e, ',');
					if (str2) {
						*str2 = '\0';
					}
					tmp = malloc(sizeof(struct opt_e_t));
					if (!tmp) {
						perror("ltrace: malloc");
						exit(1);
					}
					tmp->name = str_e;
					tmp->next = opt_e;
					opt_e = tmp;
					if (str2) {
						str_e = str2 + 1;
					} else {
						break;
					}
				}
				break;
			}
		case 'f':
			options.follow = 1;
			break;
		case 'F':
			{
				struct opt_F_t *tmp = malloc(sizeof(struct opt_F_t));
				if (!tmp) {
					perror("ltrace: malloc");
					exit(1);
				}
				tmp->filename = strdup(optarg);
				tmp->next = opt_F;
				opt_F = tmp;
				break;
			}
		case 'h':
			usage();
			exit(0);
		case 'i':
			opt_i++;
			break;
		case 'l':
			if (library_num == MAX_LIBRARY) {
				fprintf(stderr,
					"Too many libraries.  Maximum is %i.\n",
					MAX_LIBRARY);
				exit(1);
			}
			library[library_num++] = optarg;
			break;
		case 'L':
			options.libcalls = 0;
			break;
		case 'n':
			options.indent = atoi(optarg);
			break;
		case 'o':
			options.output = fopen(optarg, "w");
			if (!options.output) {
				fprintf(stderr,
					"Can't open %s for output: %s\n",
					optarg, strerror(errno));
				exit(1);
			}
			setvbuf(options.output, (char *)NULL, _IOLBF, 0);
			fcntl(fileno(options.output), F_SETFD, FD_CLOEXEC);
			break;
		case 'p':
			{
				struct opt_p_t *tmp = malloc(sizeof(struct opt_p_t));
				if (!tmp) {
					perror("ltrace: malloc");
					exit(1);
				}
				tmp->pid = atoi(optarg);
				tmp->next = opt_p;
				opt_p = tmp;
				break;
			}
		case 'r':
			opt_r++;
			break;
		case 's':
			options.strlen = atoi(optarg);
			break;
		case 'S':
			options.syscalls = 1;
			break;
		case 't':
			opt_t++;
			break;
		case 'T':
			opt_T++;
			break;
		case 'u':
			options.user = optarg;
			break;
		case 'V':
			printf("ltrace version " PACKAGE_VERSION ".\n"
					"Copyright (C) 1997-2009 Juan Cespedes <cespedes@debian.org>.\n"
					"This is free software; see the GNU General Public Licence\n"
					"version 2 or later for copying conditions.  There is NO warranty.\n");
			exit(0);
		case 'X':
#ifdef PLT_REINITALISATION_BP
			PLTs_initialized_by_here = optarg;
#else
			fprintf(stderr, "WARNING: \"-X\" not used for this "
				"architecture: assuming you meant \"-x\"\n");
#endif
			/* Fall Thru */

		case 'x':
			{
				struct opt_x_t *p = opt_x;

				/* First, check for duplicate. */
				while (p && strcmp(p->name, optarg)) {
					p = p->next;
				}
				if (p) {
					break;
				}

				/* If not duplicate, add to list. */
				p = malloc(sizeof(struct opt_x_t));
				if (!p) {
					perror("ltrace: malloc");
					exit(1);
				}
				p->name = optarg;
				p->found = 0;
				p->next = opt_x;
				opt_x = p;
				break;
			}

		default:
#if HAVE_GETOPT_LONG
			fprintf(stderr,
				"Try `%s --help' for more information\n",
				progname);
#else
			fprintf(stderr, "Try `%s -h' for more information\n",
				progname);
#endif
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;
#endif

	if (!opt_F) {
		opt_F = malloc(sizeof(struct opt_F_t));
		opt_F->next = malloc(sizeof(struct opt_F_t));
		opt_F->next->next = NULL;
		opt_F->filename = USER_CONFIG_FILE;
		opt_F->next->filename = SYSTEM_CONFIG_FILE;
	}
	/* Reverse the config file list since it was built by
	 * prepending, and it would make more sense to process the
	 * files in the order they were given. Probably it would make
	 * more sense to keep a tail pointer instead? */
	{
		struct opt_F_t *egg = NULL;
		struct opt_F_t *chicken;
		while (opt_F) {
			chicken = opt_F->next;
			opt_F->next = egg;
			egg = opt_F;
			opt_F = chicken;
		}
		opt_F = egg;
	}

	if (!opt_p && argc < 1) {
		fprintf(stderr, "%s: too few arguments\n", progname);
		usage();
		exit(1);
	}
	if (opt_r && opt_t) {
		fprintf(stderr, "%s: Incompatible options -r and -t\n",
			progname);
		exit(1);
	}
	if (argc > 0) {
		command = search_for_command(argv[0]);
	}
	return &argv[0];
}
