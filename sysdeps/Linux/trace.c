#include <stdio.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/unistd.h>

#include "ltrace.h"
#include "options.h"

/* Returns 1 if a new child is about to be created
   (ie, with fork() or clone())
   Returns 0 otherwise. */
int child_p(int sysnum)
{
	return (sysnum == __NR_fork || sysnum == __NR_clone);
}

void trace_me(void)
{
	if (ptrace(PTRACE_TRACEME, 0, 1, 0)<0) {
		perror("PTRACE_TRACEME");
		exit(1);
	}
}

void continue_after_signal(pid_t pid, int signum)
{
	/* We should always trace syscalls to be able to control fork(), clone(), execve()... */
#if 0
	if (opt_S) {
		ptrace(PTRACE_SYSCALL, pid, 1, signum);
	} else {
		ptrace(PTRACE_CONT, pid, 1, signum);
	}
#else
	ptrace(PTRACE_SYSCALL, pid, 1, signum);
#endif
}

void continue_process(pid_t pid)
{
	continue_after_signal(pid, 0);
}

void continue_enabling_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	insert_breakpoint(pid, sbp);
	continue_process(pid);
}
