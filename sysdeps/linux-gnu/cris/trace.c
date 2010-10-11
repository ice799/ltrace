#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <elf.h>

#include "common.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void get_arch_dep(Process *proc)
{
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
#define SYSCALL_INSN   0xe93d
int syscall_p(Process *proc, int status, int *sysnum)
{
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		long pc = (long)get_instruction_pointer(proc);
		unsigned int insn =
		    (int)ptrace(PTRACE_PEEKTEXT, proc->pid, pc - sizeof(long),
				0);

		if ((insn >> 16) == SYSCALL_INSN) {
			*sysnum =
			    (int)ptrace(PTRACE_PEEKUSER, proc->pid,
					sizeof(long) * PT_R9, 0);
			if (proc->callstack_depth > 0
			    && proc->callstack[proc->callstack_depth -
					       1].is_syscall) {
				return 2;
			}
			return 1;
		}
	}
	return 0;
}

long gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info)
{
	int pid = proc->pid;

	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, pid, PT_R10 * 4, 0);
	} else if (arg_num < 6) {
		int pt_arg[6] =
			{
				PT_ORIG_R10, PT_R11, PT_R12, PT_R13, PT_MOF,
				PT_SRP
			};
		return ptrace(PTRACE_PEEKUSER, pid, pt_arg[arg_num] * 4, 0);
	} else {
		return ptrace(PTRACE_PEEKDATA, pid,
			      proc->stack_pointer + 4 * (arg_num - 6), 0);
	}
	return 0;
}

void save_register_args(enum tof type, Process *proc)
{
}
