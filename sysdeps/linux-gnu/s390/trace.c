/*
** S390 specific part of trace.c
**
** Other routines are in ../trace.c and need to be combined
** at link time with this code.
**
** Copyright (C) 2001,2005 IBM Corp.
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "main.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void
get_arch_dep(Process *proc) {
#ifdef __s390x__
	unsigned long psw;

	if (proc->arch_ptr)
		return;

	psw = ptrace(PTRACE_PEEKUSER, proc->pid, PT_PSWMASK, 0);

	if ((psw & 0x000000180000000) == 0x000000080000000) {
		proc->mask_32bit = 1;
		proc->personality = 1;
	}

	proc->arch_ptr = (void *)1;
#endif
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int
syscall_p(Process *proc, int status, int *sysnum) {
	long pc, opcode, offset_reg, scno, tmp;
	void *svc_addr;
	int gpr_offset[16] = { PT_GPR0, PT_GPR1, PT_ORIGGPR2, PT_GPR3,
		PT_GPR4, PT_GPR5, PT_GPR6, PT_GPR7,
		PT_GPR8, PT_GPR9, PT_GPR10, PT_GPR11,
		PT_GPR12, PT_GPR13, PT_GPR14, PT_GPR15
	};

	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {

		/*
		 * If we have PTRACE_O_TRACESYSGOOD and we have the new style
		 * of passing the system call number to user space via PT_GPR2
		 * then the task is quite easy.
		 */

		*sysnum = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR2, 0);

		if (proc->tracesysgood) {
			/* System call was encountered... */
			if (proc->callstack_depth > 0 &&
			    proc->callstack[proc->callstack_depth -
					    1].is_syscall) {
				/* syscall exit */
				*sysnum =
				    proc->callstack[proc->callstack_depth -
						    1].c_un.syscall;
				return 2;
			} else {
				/* syscall enter */
				if (*sysnum != -ENOSYS)
					return 1;
			}
		}

		/*
		 * At least one of the two requirements mentioned above is not
		 * met. Therefore the fun part starts here:
		 * We try to do some instruction decoding without even knowing
		 * the instruction code length of the last instruction executed.
		 * Needs to be done to get the system call number or to decide
		 * if we reached a breakpoint or even checking for a completely
		 * unrelated instruction.
		 * Just a heuristic that most of the time appears to work...
		 */

		pc = ptrace(PTRACE_PEEKUSER, proc->pid, PT_PSWADDR, 0);
		opcode = ptrace(PTRACE_PEEKTEXT, proc->pid,
				(char *)(pc - sizeof(long)), 0);

		if ((opcode & 0xffff) == 0x0001) {
			/* Breakpoint */
			return 0;
		} else if ((opcode & 0xff00) == 0x0a00) {
			/* SVC opcode */
			scno = opcode & 0xff;
		} else if ((opcode & 0xff000000) == 0x44000000) {
			/* Instruction decoding of EXECUTE... */
			svc_addr = (void *)(opcode & 0xfff);

			offset_reg = (opcode & 0x000f0000) >> 16;
			if (offset_reg)
				svc_addr += ptrace(PTRACE_PEEKUSER, proc->pid,
						   gpr_offset[offset_reg], 0);

			offset_reg = (opcode & 0x0000f000) >> 12;
			if (offset_reg)
				svc_addr += ptrace(PTRACE_PEEKUSER, proc->pid,
						   gpr_offset[offset_reg], 0);

			scno = ptrace(PTRACE_PEEKTEXT, proc->pid, svc_addr, 0);
#ifdef __s390x__
			scno >>= 48;
#else
			scno >>= 16;
#endif
			if ((scno & 0xff00) != 0x0a000)
				return 0;

			tmp = 0;
			offset_reg = (opcode & 0x00f00000) >> 20;
			if (offset_reg)
				tmp = ptrace(PTRACE_PEEKUSER, proc->pid,
					     gpr_offset[offset_reg], 0);

			scno = (scno | tmp) & 0xff;
		} else {
			/* No opcode related to syscall handling */
			return 0;
		}

		if (scno == 0)
			scno = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR1, 0);

		*sysnum = scno;

		/* System call was encountered... */
		if (proc->callstack_depth > 0 &&
		    proc->callstack[proc->callstack_depth - 1].is_syscall) {
			return 2;
		} else {
			return 1;
		}
	}
	/* Unknown status... */
	return 0;
}

long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	long ret;

	switch (arg_num) {
	case -1:		/* return value */
		ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR2, 0);
		break;
	case 0:
		ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_ORIGGPR2, 0);
		break;
	case 1:
		ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR3, 0);
		break;
	case 2:
		ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR4, 0);
		break;
	case 3:
		ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR5, 0);
		break;
	case 4:
		ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR6, 0);
		break;
	default:
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(2);
	}
#ifdef __s390x__
	if (proc->mask_32bit)
		ret &= 0xffffffff;
#endif
	return ret;
}

void
save_register_args(enum tof type, Process *proc) {
}
