#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <string.h>

#include "common.h"
#include "ptrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void
get_arch_dep(Process *proc) {
        proc_archdep *a;

	if (!proc->arch_ptr)
		proc->arch_ptr = (void *)malloc(sizeof(proc_archdep));

	a = (proc_archdep *) (proc->arch_ptr);
	a->valid = (ptrace(PTRACE_GETREGS, proc->pid, 0, &a->regs) >= 0);
	if (a->valid) {
		a->valid = (ptrace(PTRACE_GETFPREGS, proc->pid, 0, &a->fpregs) >= 0);
	}
	if (a->regs.cs == 0x23) {
		proc->mask_32bit = 1;
		proc->personality = 1;
	}
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int
syscall_p(Process *proc, int status, int *sysnum) {
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, 8 * ORIG_RAX, 0);

		if (proc->callstack_depth > 0 &&
				proc->callstack[proc->callstack_depth - 1].is_syscall &&
				proc->callstack[proc->callstack_depth - 1].c_un.syscall == *sysnum) {
			return 2;
		}

		if (*sysnum >= 0) {
			return 1;
		}
	}
	return 0;
}

static unsigned int
gimme_arg32(enum tof type, Process *proc, int arg_num) {
	proc_archdep *a = (proc_archdep *) proc->arch_ptr;

	if (arg_num == -1) {	/* return value */
		return a->regs.rax;
	}

	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		return ptrace(PTRACE_PEEKTEXT, proc->pid,
			      proc->stack_pointer + 4 * (arg_num + 1), 0);
	} else if (type == LT_TOF_SYSCALL || type == LT_TOF_SYSCALLR) {
		switch (arg_num) {
		case 0:
			return a->regs.rbx;
		case 1:
			return a->regs.rcx;
		case 2:
			return a->regs.rdx;
		case 3:
			return a->regs.rsi;
		case 4:
			return a->regs.rdi;
		case 5:
			return a->regs.rbp;
		default:
			fprintf(stderr,
				"gimme_arg32 called with wrong arguments\n");
			exit(2);
		}
	}
	fprintf(stderr, "gimme_arg called with wrong arguments\n");
	exit(1);
}

long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	proc_archdep *a = (proc_archdep *) proc->arch_ptr;

	if (!a || !a->valid)
		return -1;

	if (proc->mask_32bit)
		return (unsigned int)gimme_arg32(type, proc, arg_num);

	if (arg_num == -1) {	/* return value */
		return a->regs.rax;
	}

	if (type == LT_TOF_FUNCTION) {
		if (info->type == ARGTYPE_FLOAT)
			return a->fpregs.xmm_space[4*arg_num];

		switch (arg_num) {
		case 0:
			return a->regs.rdi;
		case 1:
			return a->regs.rsi;
		case 2:
			return a->regs.rdx;
		case 3:
			return a->regs.rcx;
		case 4:
			return a->regs.r8;
		case 5:
			return a->regs.r9;
		default:
			return ptrace(PTRACE_PEEKTEXT, proc->pid,
				      proc->stack_pointer + 8 * (arg_num - 6 +
								 1), 0);
		}
	} else if (type == LT_TOF_FUNCTIONR) {
		if (info->type == ARGTYPE_FLOAT)
			return a->func_fpr_args[4*arg_num];
		switch (arg_num) {
			case 0 ... 5:
				return a->func_args[arg_num];
			default:
				return ptrace(PTRACE_PEEKTEXT, proc->pid,
				      proc->stack_pointer + 8 * (arg_num - 6 +
								 1), 0);
		}
	} else if (type == LT_TOF_SYSCALL || LT_TOF_SYSCALLR) {
		switch (arg_num) {
		case 0:
			return a->regs.rdi;
		case 1:
			return a->regs.rsi;
		case 2:
			return a->regs.rdx;
		case 3:
			return a->regs.rcx;
		case 4:
			return a->regs.r8;
		case 5:
			return a->regs.r9;
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

void
save_register_args(enum tof type, Process *proc) {
	proc_archdep *a = (proc_archdep *) proc->arch_ptr;

	if (a && a->valid) {
		if(type == LT_TOF_FUNCTION) {
			a->func_args[0] = a->regs.rdi;
			a->func_args[1] = a->regs.rsi;
			a->func_args[2] = a->regs.rdx;
			a->func_args[3] = a->regs.rcx;
			a->func_args[4] = a->regs.r8;
			a->func_args[5] = a->regs.r9;
			memcpy(a->func_fpr_args, &a->fpregs.xmm_space, sizeof(a->func_fpr_args));

		} else {
			a->sysc_args[0] = a->regs.rdi;
			a->sysc_args[1] = a->regs.rsi;
			a->sysc_args[2] = a->regs.rdx;
			a->sysc_args[3] = a->regs.rcx;
			a->sysc_args[4] = a->regs.r8;
			a->sysc_args[5] = a->regs.r9;
		}
	}
}
