#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <elf.h>
#include <errno.h>
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
	if (proc->arch_ptr == NULL) {
		proc->arch_ptr = malloc(sizeof(proc_archdep));
#ifdef __powerpc64__
		proc->mask_32bit = (proc->e_machine == EM_PPC);
#endif
	}

	proc_archdep *a = (proc_archdep *) (proc->arch_ptr);
	a->valid = (ptrace(PTRACE_GETREGS, proc->pid, 0, &a->regs) >= 0)
		&& (ptrace(PTRACE_GETFPREGS, proc->pid, 0, &a->fpregs) >= 0);
}

#define SYSCALL_INSN   0x44000002

unsigned int greg = 3;
unsigned int freg = 1;

/* Returns 1 if syscall, 2 if sysret, 0 otherwise. */
int
syscall_p(Process *proc, int status, int *sysnum) {
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		long pc = (long)get_instruction_pointer(proc);
		int insn =
		    (int)ptrace(PTRACE_PEEKTEXT, proc->pid, pc - sizeof(long),
				0);

		if (insn == SYSCALL_INSN) {
			*sysnum =
			    (int)ptrace(PTRACE_PEEKUSER, proc->pid,
					sizeof(long) * PT_R0, 0);
			if (proc->callstack_depth > 0 &&
					proc->callstack[proc->callstack_depth - 1].is_syscall &&
					proc->callstack[proc->callstack_depth - 1].c_un.syscall == *sysnum) {
				return 2;
			}
			return 1;
		}
	}
	return 0;
}

static long
gimme_arg_regset(enum tof type, Process *proc, int arg_num, arg_type_info *info,
		 gregset_t *regs, fpregset_t *fpregs)
{
	union { long val; float fval; double dval; } cvt;

	if (info->type == ARGTYPE_FLOAT || info->type == ARGTYPE_DOUBLE) {
		if (freg <= 13 || (proc->mask_32bit && freg <= 8)) {
			double val = (*fpregs)[freg];

			if (info->type == ARGTYPE_FLOAT)
				cvt.fval = val;
			else
				cvt.dval = val;

			freg++;
			greg++;

			return cvt.val;
		}
	}
	else if (greg <= 10)
		return (*regs)[greg++];
	else
		return ptrace (PTRACE_PEEKDATA, proc->pid,
				proc->stack_pointer + sizeof (long) *
				(arg_num - 8), 0);

	return 0;
}

static long
gimme_retval(Process *proc, int arg_num, arg_type_info *info,
	     gregset_t *regs, fpregset_t *fpregs)
{
	union { long val; float fval; double dval; } cvt;
	if (info->type == ARGTYPE_FLOAT || info->type == ARGTYPE_DOUBLE) {
		double val = (*fpregs)[1];

		if (info->type == ARGTYPE_FLOAT)
			cvt.fval = val;
		else
			cvt.dval = val;

		return cvt.val;
	}
	else 
		return (*regs)[3];
}

/* Grab functions arguments based on the PPC64 ABI.  */
long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info)
{
	proc_archdep *arch = (proc_archdep *)proc->arch_ptr;
	if (arch == NULL || !arch->valid)
		return -1;

	/* Check if we're entering a new function call to list parameters.  If
	   so, initialize the register control variables to keep track of where
	   the parameters were stored.  */
	if ((type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR)
	    && arg_num == 0) {
		/* Initialize the set of registrers for parameter passing.  */
		greg = 3;
		freg = 1;
	}


	if (type == LT_TOF_FUNCTIONR) {
		if (arg_num == -1)
			return gimme_retval(proc, arg_num, info,
					    &arch->regs, &arch->fpregs);
		else
			return gimme_arg_regset(type, proc, arg_num, info,
						&arch->regs_copy,
						&arch->fpregs_copy);
	}
	else
		return gimme_arg_regset(type, proc, arg_num, info,
					&arch->regs, &arch->fpregs);
}

void
save_register_args(enum tof type, Process *proc) {
	proc_archdep *arch = (proc_archdep *)proc->arch_ptr;
	if (arch == NULL || !arch->valid)
		return;

	memcpy(&arch->regs_copy, &arch->regs, sizeof(arch->regs));
	memcpy(&arch->fpregs_copy, &arch->fpregs, sizeof(arch->fpregs));
}

/* Read a single long from the process's memory address 'addr'.  */
int
arch_umovelong (Process *proc, void *addr, long *result, arg_type_info *info) {
	long pointed_to;

	errno = 0;

	pointed_to = ptrace (PTRACE_PEEKTEXT, proc->pid, addr, 0);

	if (pointed_to == -1 && errno)
		return -errno;

	/* Since int's are 4-bytes (long is 8-bytes) in length for ppc64, we
	   need to shift the long values returned by ptrace to end up with
	   the correct value.  */

	if (info) {
		if (info->type == ARGTYPE_INT || (proc->mask_32bit && (info->type == ARGTYPE_POINTER
		    || info->type == ARGTYPE_STRING))) {
			pointed_to = pointed_to >> 32;

			/* Make sure we have nothing in the upper word so we can
			   do a explicit cast from long to int later in the code.  */
			pointed_to &= 0x00000000ffffffff;
		}
	}

	*result = pointed_to;
	return 0;
}
