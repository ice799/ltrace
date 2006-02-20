#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <elf.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void get_arch_dep(struct process *proc)
{
#ifdef __powerpc64__
    if (proc->arch_ptr)
        return;
    proc->mask_32bit = (proc->e_machine == EM_PPC);
    proc->arch_ptr = (void *) 1;
#endif
}


/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
#define SYSCALL_INSN   0x44000002
int
syscall_p(struct process * proc, int status, int * sysnum) {
	if (WIFSTOPPED(status) && WSTOPSIG(status)==(SIGTRAP | proc->tracesysgood)) {
		long pc = (long)get_instruction_pointer(proc);
		int insn = (int)ptrace(PTRACE_PEEKTEXT, proc->pid, pc-sizeof(long), 0);

		if (insn == SYSCALL_INSN) {
			*sysnum = (int)ptrace(PTRACE_PEEKUSER, proc->pid, sizeof(long)*PT_R0, 0);
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
		return ptrace(PTRACE_PEEKUSER, proc->pid, sizeof(long)*PT_R3, 0);
	} else if (arg_num < 8) {
		return ptrace(PTRACE_PEEKUSER, proc->pid, sizeof(long)*(arg_num+PT_R3), 0);
	} else {
		return ptrace(PTRACE_PEEKDATA, proc->pid, proc->stack_pointer+8*(arg_num-8), 0);
	}
	return 0;
}

void
save_register_args(enum tof type, struct process * proc) {
}
