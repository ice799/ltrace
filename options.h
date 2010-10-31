#include <stdio.h>
#include <sys/types.h>

struct options_t {
	int align;      /* -a: default alignment column for results */
	char * user;    /* -u: username to run command as */
	int syscalls;   /* -S: display system calls */
	int libcalls;   /* -L: display library calls */
	int demangle;   /* -C: demangle low-level names into user-level names */
	int indent;     /* -n: indent trace output according to program flow */
	FILE *output;   /* output to a specific file */
	int summary;    /* count time, calls, and report a summary on program exit */
	int debug;      /* debug */
	int arraylen;   /* default maximum # of array elements printed */
	int strlen;     /* default maximum # of bytes printed in strings */
	int follow;     /* trace child processes */
	int no_plt;     /* set bps on PLT entries */
	int no_signals; /* don't print signals */
};
extern struct options_t options;

extern int opt_i;		/* instruction pointer */
extern int opt_r;		/* print relative timestamp */
extern int opt_t;		/* print absolute timestamp */
extern int opt_T;		/* show the time spent inside each call */

struct opt_p_t {
	pid_t pid;
	struct opt_p_t *next;
};

struct opt_e_t {
	char *name;
	struct opt_e_t *next;
};

struct opt_F_t {
	char *filename;
	struct opt_F_t *next;
};

struct opt_x_t {
	char *name;
	int found;
	struct opt_x_t *next;
};

extern struct opt_p_t *opt_p;	/* attach to process with a given pid */

extern struct opt_e_t *opt_e;	/* list of function names to display */
extern int opt_e_enable;	/* 0 if '!' is used, 1 otherwise */

extern struct opt_F_t *opt_F;	/* alternate configuration file(s) */

extern struct opt_x_t *opt_x;	/* list of functions to break at */

extern char **process_options(int argc, char **argv);
