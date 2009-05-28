#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "main.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void
get_arch_dep(Process *proc) {
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int
syscall_p(Process *proc, int status, int *sysnum) {
	int depth;

	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_ORIG_D0, 0);
		if (*sysnum == -1)
			return 0;
		if (*sysnum >= 0) {
			depth = proc->callstack_depth;
			if (depth > 0 &&
					proc->callstack[depth - 1].is_syscall &&
					proc->callstack[depth - 1].c_un.syscall == *sysnum) {
				return 2;
			} else {
				return 1;
			}
		}
	}
	return 0;
}

long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_D0, 0);
	}

	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		return ptrace(PTRACE_PEEKTEXT, proc->pid,
			      proc->stack_pointer + 4 * (arg_num + 1), 0);
	} else if (type == LT_TOF_SYSCALL || type == LT_TOF_SYSCALLR) {
#if 0
		switch (arg_num) {
		case 0:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_D1, 0);
		case 1:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_D2, 0);
		case 2:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_D3, 0);
		case 3:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_D4, 0);
		case 4:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_D5, 0);
		default:
			fprintf(stderr,
				"gimme_arg called with wrong arguments\n");
			exit(2);
		}
#else
		/* That hack works on m68k, too */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * arg_num, 0);
#endif
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}

	return 0;
}

void
save_register_args(enum tof type, Process *proc) {
}
