#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ltrace.h"
#include "options.h"
#include "defs.h"

static char *progname;		/* Program name (`ltrace') */
FILE * output;
int opt_a = DEFAULT_ACOLUMN;	/* default alignment column for results */
int opt_d = 0;			/* debug */
int opt_i = 0;			/* instruction pointer */
int opt_s = DEFAULT_STRLEN;	/* default maximum # of bytes printed in strings */
int opt_S = 0;			/* display syscalls */
int opt_L = 1;			/* display library calls */
int opt_f = 0;			/* trace child processes as they are created */
char * opt_u = NULL;		/* username to run command as */
int opt_r = 0;			/* print relative timestamp */
int opt_t = 0;			/* print absolute timestamp */
#if HAVE_LIBIBERTY
int opt_C = 0;			/* Demangle low-level symbol names into user-level names */
#endif

/* List of pids given to option -p: */
struct opt_p_t * opt_p = NULL;	/* attach to process with a given pid */

/* List of function names given to option -e: */
struct opt_e_t * opt_e = NULL;
int opt_e_enable=1;

static void usage(void)
{
#if !(HAVE_GETOPT || HAVE_GETOPT_LONG)
	fprintf(stdout, "Usage: %s [command [arg ...]]\n"
"Trace library calls of a given program.\n\n", progname);
#else
	fprintf(stdout, "Usage: %s [option ...] [command [arg ...]]\n"
"Trace library calls of a given program.\n\n"

# if HAVE_GETOPT_LONG
"  -d, --debug         print debugging info.\n"
# else
"  -d                  print debugging info.\n"
# endif
"  -f                  follow forks.\n"
"  -i                  print instruction pointer at time of library call.\n"
"  -L                  do NOT display library calls.\n"
"  -S                  display system calls.\n"
"  -r                  print relative timestamps.\n"
"  -t, -tt, -ttt       print absolute timestamps.\n"
# if HAVE_LIBIBERTY
#  if HAVE_GETOPT_LONG
"  -C, --demangle      decode low-level symbol names into user-level names.\n"
#  else
"  -C                  decode low-level symbol names into user-level names.\n"
#  endif
# endif
# if HAVE_GETOPT_LONG
"  -a, --align=COLUMN  align return values in a secific column.\n"
# else
"  -a COLUMN           align return values in a secific column.\n"
# endif
"  -s STRLEN           specify the maximum string size to print.\n"
# if HAVE_GETOPT_LONG
"  -o, --output=FILE   write the trace output to that file.\n"
# else
"  -o FILE             write the trace output to that file.\n"
# endif
"  -u USERNAME         run command with the userid, groupid of username.\n"
"  -p PID              attach to the process with the process ID pid.\n"
"  -e expr             modify which events to trace.\n"
# if HAVE_GETOPT_LONG
"  -h, --help          display this help and exit.\n"
# else
"  -h                  display this help and exit.\n"
# endif
# if HAVE_GETOPT_LONG
"  -V, --version       output version information and exit.\n"
# else
"  -V                  output version information and exit.\n"
# endif
"\nReport bugs to Juan Cespedes <cespedes@debian.org>\n"
		, progname);
#endif
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
	progname = argv[0];
	output = stderr;

#if HAVE_GETOPT || HAVE_GETOPT_LONG
	while(1) {
		int c;
#if HAVE_GETOPT_LONG
		int option_index = 0;
		static struct option long_options[] = {
			{ "align", 1, 0, 'a'},
			{ "debug", 0, 0, 'd'},
			{ "demangle", 0, 0, 'C'},
			{ "help", 0, 0, 'h'},
			{ "output", 1, 0, 'o'},
			{ "version", 0, 0, 'V'},
			{ 0, 0, 0, 0}
		};
		c = getopt_long(argc, argv, "+dfiLSrthV"
# if HAVE_LIBIBERTY
			"C"
# endif
			"a:s:o:u:p:e:", long_options, &option_index);
#else
		c = getopt(argc, argv, "+dfiLSth"
# if HAVE_LIBIBERTY
			"C"
# endif
			"a:s:o:u:p:e:");
#endif
		if (c==-1) {
			break;
		}
		switch(c) {
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
			case 'r':	opt_r++;
					break;
			case 't':	opt_t++;
					break;
#if HAVE_LIBIBERTY
			case 'C':	opt_C++;
					break;
#endif
			case 'a':	opt_a = atoi(optarg);
					break;
			case 's':	opt_s = atoi(optarg);
					break;
			case 'h':	usage();
					exit(0);
			case 'o':	output = fopen(optarg, "w");
					if (!output) {
						fprintf(stderr, "Can't open %s for output: %s\n", optarg, strerror(errno));
						exit(1);
					}
					setvbuf(output, (char *)NULL, _IOLBF, 0);
					break;
			case 'u':	opt_u = optarg;
					break;
			case 'p':	
				{
					struct opt_p_t * tmp = malloc(sizeof(struct opt_p_t));
					if (!tmp) {
						perror("malloc");
						exit(1);
					}
					tmp->pid = atoi(optarg);
					tmp->next = opt_p;
					opt_p = tmp;
					break;
				}
			case 'e':
				{
					char * str_e = strdup(optarg);
					if (!str_e) {
						perror("strdup");
						exit(1);
					}
					if (str_e[0]=='!') {
						opt_e_enable=0;
						str_e++;
					}
					while(*str_e) {
						struct opt_e_t * tmp;
						char *str2 = strchr(str_e,',');
						if (str2) {
							*str2 = '\0';
						}
						tmp = malloc(sizeof(struct opt_e_t));
						if (!tmp) {
							perror("malloc");
							exit(1);
						}
						tmp->name = str_e;
						tmp->next = opt_e;
						opt_e = tmp;
						if (str2) {
							str_e = str2+1;
						} else {
							break;
						}
					}
					break;
				}
			case 'V':	printf("ltrace version "
#ifdef VERSION
					VERSION
#else
					"???"
#endif
					".\n"
"Copyright (C) 1997-1998 Juan Cespedes <cespedes@debian.org>.\n"
"This is free software; see the GNU General Public Licence\n"
"version 2 or later for copying conditions.  There is NO warranty.\n");
					exit(0);

			default:
#if HAVE_GETOPT_LONG
				fprintf(stderr, "Try `%s --help' for more information\n", progname);
#else
				fprintf(stderr, "Try `%s -h' for more information\n", progname);
#endif
					exit(1);
		}
	}
	argc -= optind; argv += optind;
#endif

	if (!opt_p && argc<1) {
		fprintf(stderr, "%s: too few arguments\n", progname);
#if HAVE_GETOPT_LONG
		fprintf(stderr, "Try `%s --help' for more information\n", progname);
#elif HAVE_GETOPT
		fprintf(stderr, "Try `%s -h' for more information\n", progname);
#endif
		exit(1);
	}
	if (opt_r && opt_t) {
		fprintf(stderr, "%s: Incompatible options -r and -t\n", progname);
		exit(1);
	}
	if (argc>0) {
		command = search_for_command(argv[0]);
	}
	return &argv[0];
}
