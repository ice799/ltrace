/*
 * This file contains low-level functions to do process tracing
 *  (ie, 'ptrace' in POSIX)
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

int attached_pid;

#include "i386.h"
#include "trace.h"

int attach_process(const char * file, char * const argv[])
{
	int status;
	attached_pid = fork();

	if (attached_pid<0) {
		perror("fork");
		exit(1);
	} else if (!attached_pid) {
		if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
			perror("PTRACE_TRACEME");
			exit(1);
		}
		execvp(file, argv);
		fprintf(stderr, "Can't execute \"%s\": %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}
	/* Wait until execve... */
	if (wait4(attached_pid, &status, 0, NULL)==-1) {
		perror("wait4");
		exit(1);
	}
	if (!(WIFSTOPPED(status) && (WSTOPSIG(status)==SIGTRAP))) {
		fprintf(stderr, "Unknown exit status for process %s\n", file);
		exit(1);
	}
	return attached_pid;
}

void continue_process(int pid, int signal)
{
	ptrace(PTRACE_CONT, pid, 1, signal);
}
