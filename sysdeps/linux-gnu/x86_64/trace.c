#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "common.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void
get_arch_dep(Process *proc) {
	unsigned long cs;
	if (proc->arch_ptr)
		return;
	cs = ptrace(PTRACE_PEEKUSER, proc->pid, 8 * CS, 0);
	if (cs == 0x23) {
		proc->mask_32bit = 1;
		proc->personality = 1;
	}
	proc->arch_ptr = (void *)1;
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
	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RAX, 0);
	}

	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		return ptrace(PTRACE_PEEKTEXT, proc->pid,
			      proc->stack_pointer + 4 * (arg_num + 1), 0);
	} else if (type == LT_TOF_SYSCALL || type == LT_TOF_SYSCALLR) {
		switch (arg_num) {
		case 0:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RBX, 0);
		case 1:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RCX, 0);
		case 2:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDX, 0);
		case 3:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RSI, 0);
		case 4:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDI, 0);
		case 5:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RBP, 0);
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
	if (proc->mask_32bit)
		return (unsigned int)gimme_arg32(type, proc, arg_num);

	if (arg_num == -1) {	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RAX, 0);
	}

	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		switch (arg_num) {
		case 0:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDI, 0);
		case 1:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RSI, 0);
		case 2:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDX, 0);
		case 3:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RCX, 0);
		case 4:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R8, 0);
		case 5:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R9, 0);
		default:
			return ptrace(PTRACE_PEEKTEXT, proc->pid,
				      proc->stack_pointer + 8 * (arg_num - 6 +
								 1), 0);
		}
	} else if (type == LT_TOF_SYSCALL || LT_TOF_SYSCALLR) {
		switch (arg_num) {
		case 0:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDI, 0);
		case 1:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RSI, 0);
		case 2:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * RDX, 0);
		case 3:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R10, 0);
		case 4:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R8, 0);
		case 5:
			return ptrace(PTRACE_PEEKUSER, proc->pid, 8 * R9, 0);
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
}

int
find_dynamic_entry_addr(Process *proc, void *pvAddr, int d_tag, void **addr) {
	int i = 0, done = 0;
	Elf64_Dyn entry;

	debug(DEBUG_FUNCTION, "find_dynamic_entry()");

	if (addr ==  NULL || pvAddr == NULL || d_tag < 0 || d_tag > DT_NUM) {
		return -1;
	}

	while ((!done) && (i < ELF_MAX_SEGMENTS) &&
			(sizeof(entry) == umovebytes(proc, pvAddr, &entry, sizeof(entry))) &&
			(entry.d_tag != DT_NULL)) {
		if (entry.d_tag == d_tag) {
			done = 1;
			*addr = (void *)entry.d_un.d_val;
		}
		pvAddr += sizeof(entry);
		i++;
	}

	if (done) {
		debug(2, "found address: 0x%p in dtag %d\n", *addr, d_tag);
		return 0;
	}
	else {
		debug(2, "Couldn't address for dtag!\n");
		return -1;
	}
}
