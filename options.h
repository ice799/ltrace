#include <stdio.h>

extern FILE * output;
extern int opt_a;	/* default alignment column for results */
extern int opt_d;	/* debug */
extern int opt_i;	/* instruction pointer */
extern int opt_s;	/* default maximum # of bytes printed in strings */
extern int opt_L;	/* display library calls */
extern int opt_S;	/* display system calls */
extern int opt_f;	/* trace child processes */
extern char * opt_u;	/* username to run command as */

struct opt_p_t {
	pid_t pid;
	struct opt_p_t * next;
};

extern struct opt_p_t * opt_p;	/* attach to process with a given pid */

extern char ** process_options(int argc, char **argv);
