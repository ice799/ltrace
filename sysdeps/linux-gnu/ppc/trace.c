#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <elf.h>
#include <errno.h>

#include "main.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void
get_arch_dep(Process *proc) {
#ifdef __powerpc64__
	if (proc->arch_ptr)
		return;
	proc->mask_32bit = (proc->e_machine == EM_PPC);
	proc->arch_ptr = (void *)1;
#endif
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise. */
#define SYSCALL_INSN   0x44000002

unsigned int greg = 3;
unsigned int freg = 1;
unsigned int vreg = 2;

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

/* Grab functions arguments based on the PPC64 ABI.  */
long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	long data;

	if (type == LT_TOF_FUNCTIONR) {
		if (info->type == ARGTYPE_FLOAT || info->type == ARGTYPE_DOUBLE)
			return ptrace (PTRACE_PEEKUSER, proc->pid,
					sizeof (long) * (PT_FPR0 + 1), 0);
		else
			return ptrace (PTRACE_PEEKUSER, proc->pid,
					sizeof (long) * PT_R3, 0);
	}

	/* Check if we're entering a new function call to list parameters.  If
	   so, initialize the register control variables to keep track of where
	   the parameters were stored.  */
	if (type == LT_TOF_FUNCTION && arg_num == 0) {
	  /* Initialize the set of registrers for parameter passing.  */
		greg = 3;
		freg = 1;
		vreg = 2;
	}

	if (info->type == ARGTYPE_FLOAT || info->type == ARGTYPE_DOUBLE) {
		if (freg <= 13 || (proc->mask_32bit && freg <= 8)) {
			data = ptrace (PTRACE_PEEKUSER, proc->pid,
					sizeof (long) * (PT_FPR0 + freg), 0);

			if (info->type == ARGTYPE_FLOAT) {
			/* float values passed in FP registers are automatically
			promoted to double. We need to convert it back to float
			before printing.  */
				union { long val; float fval; double dval; } cvt;
				cvt.val = data;
				cvt.fval = (float) cvt.dval;
				data = cvt.val;
			}

			freg++;
			greg++;

			return data;
		}
	}
	else if (greg <= 10) {
		data = ptrace (PTRACE_PEEKUSER, proc->pid,
				sizeof (long) * greg, 0);
		greg++;

		return data;
	}
	else
		return ptrace (PTRACE_PEEKDATA, proc->pid,
				proc->stack_pointer + sizeof (long) *
				(arg_num - 8), 0);

	return 0;
}

void
save_register_args(enum tof type, Process *proc) {
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
