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
	unsigned long cs;
	if (proc->arch_ptr)
		return;
	cs = ptrace(PTRACE_PEEKUSER, proc->pid, 8*CS, 0);
	if (cs == 0x23) {
		proc->mask_32bit = 1;
		proc->personality = 1;
	}
	proc->arch_ptr = (void *) 1;
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int
syscall_p(struct process * proc, int status, int * sysnum) {
	if (WIFSTOPPED(status) && WSTOPSIG(status)==(SIGTRAP | proc->tracesysgood)) {
		*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, 8*ORIG_RAX, 0);

		if (proc->callstack_depth > 0 &&
				proc->callstack[proc->callstack_depth-1].is_syscall) {
			return 2;
		}

		if (*sysnum>=0) {
			return 1;
		}
	}
	return 0;
}

static unsigned int
gimme_arg32(enum tof type, struct process * proc, int arg_num) {
	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RAX, 0);
	}

	if (type==LT_TOF_FUNCTION || type==LT_TOF_FUNCTIONR) {
		return ptrace(PTRACE_PEEKTEXT, proc->pid, proc->stack_pointer+4*(arg_num+1), 0);
	} else if (type==LT_TOF_SYSCALL || type==LT_TOF_SYSCALLR) {
		switch(arg_num) {
			case 0: return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RBX, 0);
			case 1: return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RCX, 0);
			case 2: return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RDX, 0);
			case 3: return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RSI, 0);
			case 4: return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RDI, 0);
			case 5: return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RBP, 0);
			default:
				fprintf(stderr, "gimme_arg32 called with wrong arguments\n");
				exit(2);
		}
	}
	fprintf(stderr, "gimme_arg called with wrong arguments\n");
	exit(1);
}

long
gimme_arg(enum tof type, struct process * proc, int arg_num) {
	if (proc->mask_32bit)
		return (unsigned int)gimme_arg32(type, proc, arg_num);

	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RAX, 0);
	}

	if (type==LT_TOF_FUNCTION || type==LT_TOF_FUNCTIONR) {
		switch(arg_num) {
			case 0:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RDI, 0);
			case 1:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RSI, 0);
			case 2:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RDX, 0);
			case 3:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RCX, 0);
			case 4:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*R8, 0);
			case 5:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*R9, 0);
			default:
				return ptrace(PTRACE_PEEKTEXT, proc->pid,
					proc->stack_pointer + 8 * (arg_num - 6 + 1), 0);
		}
	} else if (type==LT_TOF_SYSCALL || LT_TOF_SYSCALLR) {
		switch(arg_num) {
			case 0:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RDI, 0);
			case 1:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RSI, 0);
			case 2:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*RDX, 0);
			case 3:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*R10, 0);
			case 4:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*R8, 0);
			case 5:	return ptrace(PTRACE_PEEKUSER, proc->pid, 8*R9, 0);
			default:
				fprintf(stderr, "gimme_arg called with wrong arguments\n");
				exit(2);
		}
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}

	return 0;
}

void save_register_args(enum tof type, struct process * proc)
{
}
