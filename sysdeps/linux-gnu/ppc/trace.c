#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
#define SYSCALL_INSN   0x44000002
int
syscall_p(struct process * proc, int status, int * sysnum) {
	if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGTRAP) {
		int pc = ptrace(PTRACE_PEEKUSER, proc->pid, 4*PT_NIP, 0);
		int insn = ptrace(PTRACE_PEEKTEXT, proc->pid, pc-4, 0);

		if (insn == SYSCALL_INSN) {
			*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, 4*PT_R0, 0);
			if (proc->callstack_depth > 0 &&
					proc->callstack[proc->callstack_depth-1].is_syscall) {
				return 2;
			}
			return 1;
		}
	}
	return 0;
}

long
gimme_arg(enum tof type, struct process * proc, int arg_num) {
	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 4*PT_R3, 0);
	}

	if (type==LT_TOF_FUNCTION || type==LT_TOF_SYSCALL) {
		if (arg_num < 8) {
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4*(arg_num+PT_R3), 0);
		} else {
			return ptrace(PTRACE_PEEKDATA, proc->pid, proc->stack_pointer+8*(arg_num-8), 0);
		}
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}
	return 0;
}
