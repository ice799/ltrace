#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void get_arch_dep(struct process *proc)
{
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int syscall_p(struct process *proc, int status, int *sysnum)
{
	if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
		*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, 8 * ORIG_RAX, 0);

		if (proc->callstack_depth > 0 &&
		    proc->callstack[proc->callstack_depth - 1].is_syscall) {
			return 2;
		}

		if (*sysnum >= 0) {
			return 1;
		}
	}
	return 0;
}

long gimme_arg(enum tof type, struct process *proc, int arg_num)
{
	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RAX, 0);
	}

	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		switch (arg_num) {
		case 0:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDI, 0);
		case 1:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RSI, 0);
		case 2:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDX, 0);
		case 3:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RCX, 0);
		case 4:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R8, 0);
		case 5:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R9, 0);
		default:
			return ptrace(PTRACE_PEEKTEXT, proc->pid,
				      proc->stack_pointer + 8 * (arg_num - 6 +
								 1), 0);
			fprintf(stderr,
				"gimme_arg called with wrong arguments\n");
			exit(2);
		}
	} else if (type == LT_TOF_SYSCALL || LT_TOF_SYSCALLR) {
		switch (arg_num) {
		case 0:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDI, 0);
		case 1:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RSI, 0);
		case 2:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDX, 0);
		case 3:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R10, 0);
		case 4:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R8, 0);
		case 5:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R9, 0);
		default:
			fprintf(stderr,
				"gimme_arg called with wrong arguments\n");
			exit(2);
		}
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}

	return 0;
}

void save_register_args(enum tof type, struct process *proc)
{
}
