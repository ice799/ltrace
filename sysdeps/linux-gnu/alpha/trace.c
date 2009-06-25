#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "common.h"
#include "debug.h"

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
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		char *ip = get_instruction_pointer(proc) - 4;
		long x = ptrace(PTRACE_PEEKTEXT, proc->pid, ip, 0);
		debug(2, "instr: %016lx", x);
		if ((x & 0xffffffff) != 0x00000083)
			return 0;
		*sysnum =
		    ptrace(PTRACE_PEEKUSER, proc->pid, 0 /* REG_R0 */ , 0);
		if (proc->callstack_depth > 0 &&
		    proc->callstack[proc->callstack_depth - 1].is_syscall &&
			proc->callstack[proc->callstack_depth - 1].c_un.syscall == *sysnum) {
			return 2;
		}
		if (*sysnum >= 0 && *sysnum < 500) {
			return 1;
		}
	}
	return 0;
}

long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 0 /* REG_R0 */ , 0);
	}

	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		if (arg_num <= 5)
			return ptrace(PTRACE_PEEKUSER, proc->pid,
				      arg_num + 16 /* REG_A0 */ , 0);
		else
			return ptrace(PTRACE_PEEKTEXT, proc->pid,
				      proc->stack_pointer + 8 * (arg_num - 6),
				      0);
	} else if (type == LT_TOF_SYSCALL || type == LT_TOF_SYSCALLR) {
		return ptrace(PTRACE_PEEKUSER, proc->pid,
			      arg_num + 16 /* REG_A0 */ , 0);
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}
	return 0;
}

void
save_register_args(enum tof type, Process *proc) {
}
