#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "common.h"
#include "output.h"
#include "ptrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

#define off_r0 0
#define off_r7 28
#define off_ip 48
#define off_pc 60

void
get_arch_dep(Process *proc) {
	proc_archdep *a;

	if (!proc->arch_ptr)
		proc->arch_ptr = (void *)malloc(sizeof(proc_archdep));
	a = (proc_archdep *) (proc->arch_ptr);
	a->valid = (ptrace(PTRACE_GETREGS, proc->pid, 0, &a->regs) >= 0);
}

/* Returns 0 if not a syscall,
 *         1 if syscall entry, 2 if syscall exit,
 *         3 if arch-specific syscall entry, 4 if arch-specific syscall exit,
 *         -1 on error.
 */
int
syscall_p(Process *proc, int status, int *sysnum) {
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		/* get the user's pc (plus 8) */
		int pc = ptrace(PTRACE_PEEKUSER, proc->pid, off_pc, 0);
		/* fetch the SWI instruction */
		int insn = ptrace(PTRACE_PEEKTEXT, proc->pid, pc - 4, 0);
		int ip = ptrace(PTRACE_PEEKUSER, proc->pid, off_ip, 0);

		if (insn == 0xef000000 || insn == 0x0f000000
		    || (insn & 0xffff0000) == 0xdf000000) {
			/* EABI syscall */
			*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, off_r7, 0);
		} else if ((insn & 0xfff00000) == 0xef900000) {
			/* old ABI syscall */
			*sysnum = insn & 0xfffff;
		} else {
			/* TODO: handle swi<cond> variations */
			/* one possible reason for getting in here is that we
			 * are coming from a signal handler, so the current
			 * PC does not point to the instruction just after the
			 * "swi" one. */
			output_line(proc, "unexpected instruction 0x%x at %p", insn, pc - 4);
			return 0;
		}
		if ((*sysnum & 0xf0000) == 0xf0000) {
			/* arch-specific syscall */
			*sysnum &= ~0xf0000;
			return ip ? 4 : 3;
		}
		/* ARM syscall convention: on syscall entry, ip is zero;
		 * on syscall exit, ip is non-zero */
		return ip ? 2 : 1;
	}
	return 0;
}

long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	proc_archdep *a = (proc_archdep *) proc->arch_ptr;

	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, off_r0, 0);
	}

	/* deal with the ARM calling conventions */
	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		if (arg_num < 4) {
			if (a->valid && type == LT_TOF_FUNCTION)
				return a->regs.uregs[arg_num];
			if (a->valid && type == LT_TOF_FUNCTIONR)
				return a->func_arg[arg_num];
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * arg_num,
				      0);
		} else {
			return ptrace(PTRACE_PEEKDATA, proc->pid,
				      proc->stack_pointer + 4 * (arg_num - 4),
				      0);
		}
	} else if (type == LT_TOF_SYSCALL || type == LT_TOF_SYSCALLR) {
		if (arg_num < 5) {
			if (a->valid && type == LT_TOF_SYSCALL)
				return a->regs.uregs[arg_num];
			if (a->valid && type == LT_TOF_SYSCALLR)
				return a->sysc_arg[arg_num];
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4 * arg_num,
				      0);
		} else {
			return ptrace(PTRACE_PEEKDATA, proc->pid,
				      proc->stack_pointer + 4 * (arg_num - 5),
				      0);
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
	if (a->valid) {
		if (type == LT_TOF_FUNCTION)
			memcpy(a->func_arg, a->regs.uregs, sizeof(a->func_arg));
		else
			memcpy(a->sysc_arg, a->regs.uregs, sizeof(a->sysc_arg));
	}
}
