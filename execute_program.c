#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"
#include "sysdep.h"

void execute_program(struct process * sp, char **argv)
{
	int pid;

	if (opt_d) {
		output_line(0, "Executing `%s'...", sp->filename);
	}

	pid = fork();
	if (pid<0) {
		perror("fork");
		exit(1);
	} else if (!pid) {	/* child */
		trace_me();
		execvp(sp->filename, argv);
		fprintf(stderr, "Can't execute `%s': %s\n", sp->filename, strerror(errno));
		exit(1);
	}

	if (opt_d) {
		output_line(0, "PID=%d", pid);
	}

	sp->pid = pid;

	return;
}
