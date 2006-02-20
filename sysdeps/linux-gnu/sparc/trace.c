#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include "ptrace.h"
#include "ltrace.h"

extern FILE *output;
extern int opt_d;

void get_arch_dep(struct process *proc)
{
	proc_archdep *a;
	if (!proc->arch_ptr)
		proc->arch_ptr = (void *)malloc(sizeof(proc_archdep));
	a = (proc_archdep *)(proc->arch_ptr);
	a->valid = (ptrace (PTRACE_GETREGS, proc->pid, &a->regs, 0) >= 0);
}

/* Returns syscall number if `pid' stopped because of a syscall.
 * Returns -1 otherwise
 */
int syscall_p(struct process *proc, int status, int *sysnum)
{
	if (WIFSTOPPED(status) && WSTOPSIG(status)==(SIGTRAP | proc->tracesysgood)) {
		void *ip = get_instruction_pointer(proc);
		unsigned int insn;
		if (ip == (void *)-1) return 0;
		insn = ptrace(PTRACE_PEEKTEXT, proc->pid, ip, 0);
		if ((insn & 0xc1f8007f) == 0x81d00010) {
			*sysnum = ((proc_archdep *)proc->arch_ptr)->regs.r_g1;
			if ((proc->callstack_depth > 0) && proc->callstack[proc->callstack_depth-1].is_syscall) {
				return 2;
			} else if(*sysnum>=0) {
				return 1;
			}
		}
	}
	return 0;
}

long gimme_arg(enum tof type, struct process * proc, int arg_num)
{
	proc_archdep * a = (proc_archdep *)proc->arch_ptr;
	if (!a->valid) {
		fprintf(stderr, "Could not get child registers\n");
		exit(1);
	}
	if (arg_num==-1)		/* return value */
		return a->regs.r_o0;

	if (type==LT_TOF_FUNCTION || type==LT_TOF_SYSCALL || arg_num >= 6) {
		if (arg_num < 6)
			return ((int *)&a->regs.r_o0)[arg_num];
		return ptrace(PTRACE_PEEKTEXT, proc->pid, proc->stack_pointer+64*(arg_num + 1));
	} else if (type==LT_TOF_FUNCTIONR)
		return a->func_arg[arg_num];
	else if (type==LT_TOF_SYSCALLR)
		return a->sysc_arg[arg_num];
	else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}
	return 0;
}

void save_register_args(enum tof type, struct process * proc)
{
	proc_archdep * a = (proc_archdep *)proc->arch_ptr;
	if (a->valid) {
		if (type == LT_TOF_FUNCTION)
			memcpy(a->func_arg, &a->regs.r_o0, sizeof(a->func_arg));
		else
			memcpy(a->sysc_arg, &a->regs.r_o0, sizeof(a->sysc_arg));
	}
}
