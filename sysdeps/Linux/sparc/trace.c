#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>

#include "ltrace.h"

/* Returns syscall number if `pid' stopped because of a syscall.
 * Returns -1 otherwise
 */
int syscall_p(pid_t pid, int status)
{
#if 0
	if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGTRAP) {
		int tmp = ptrace(PTRACE_PEEKUSER, pid, 4*ORIG_EAX);
		if (tmp>=0) {
			return tmp;
		}
	}
#endif
	return -1;
}

void continue_after_breakpoint(struct process *proc, struct breakpoint * sbp, int delete_it)
{
	delete_breakpoint(proc->pid, sbp);
	ptrace(PTRACE_POKEUSER, proc->pid, PT_PC, sbp->addr);
	if (delete_it) {
		continue_process(proc->pid);
	} else {
		proc->breakpoint_being_enabled = sbp;
		ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0);
	}
}

long gimme_arg(enum tof type, struct process * proc, int arg_num)
{
#if 0
	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EAX);
	}

	if (type==LT_TOF_FUNCTION) {
		return ptrace(PTRACE_PEEKTEXT, proc->pid, proc->stack_pointer+4*(arg_num+1));
	} else if (type==LT_TOF_SYSCALL) {
#if 0
		switch(arg_num) {
			case 0:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EBX);
			case 1:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*ECX);
			case 2:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EDX);
			case 3:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*ESI);
			case 4:	return ptrace(PTRACE_PEEKUSER, proc->pid, 4*EDI);
			default:
				fprintf(stderr, "gimme_arg called with wrong arguments\n");
				exit(2);
		}
#else
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4*arg_num);
#endif
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}
#endif
	return 0;
}
