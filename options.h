#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

extern FILE * output;
extern int opt_a;	/* default alignment column for results */
extern int opt_c;	/* count time, calls, and report a summary on program exit */
extern int opt_d;	/* debug */
extern int opt_i;	/* instruction pointer */
extern int opt_s;	/* default maximum # of bytes printed in strings */
extern int opt_L;	/* display library calls */
extern int opt_S;	/* display system calls */
extern int opt_f;	/* trace child processes */
extern char * opt_u;	/* username to run command as */
extern int opt_r;	/* print relative timestamp */
extern int opt_t;	/* print absolute timestamp */
#if HAVE_LIBIBERTY
extern int opt_C;	/* Demanglelow-level symbol names into user-level names */
#endif
extern int opt_n;	/* indent trace output according to program flow */
extern int opt_T;	/* show the time spent inside each call */

struct opt_p_t {
	pid_t pid;
	struct opt_p_t * next;
};

struct opt_e_t {
	char * name;
	struct opt_e_t * next;
};

extern struct opt_p_t * opt_p;	/* attach to process with a given pid */

extern struct opt_e_t * opt_e;	/* list of function names to display */
extern int opt_e_enable;	/* 0 if '!' is used, 1 otherwise */

extern char ** process_options(int argc, char **argv);
