/*
** S390 specific part of trace.c
**
** Other routines are in ../trace.c and need to be combined
** at link time with this code.
**
** S/390 version
** (C) Copyright 2001 IBM Poughkeepsie, IBM Corporation
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int
syscall_p(struct process * proc, int status, int * sysnum) {
	long pswa;
	long svcinst;
	long svcno;
	long svcop;

	if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGTRAP) {

		pswa = ptrace(PTRACE_PEEKUSER, proc->pid, PT_PSWADDR, 0);
		svcinst = ptrace(PTRACE_PEEKTEXT, proc->pid, (char *)(pswa-4),0);
		svcop = (svcinst >> 8) & 0xFF;
		svcno = svcinst & 0xFF;

		*sysnum = svcno;

		if (*sysnum == -1) {
			return 0;
		}
		if (svcop == 0 && svcno == 1) {
			/* Breakpoint was hit... */
			return 0;
		}
		if (svcop == 10 && *sysnum>=0) {
			/* System call was encountered... */
			if (proc->callstack_depth > 0 &&
					proc->callstack[proc->callstack_depth-1].is_syscall) {
				return 2;
			} else {
				return 1;
			}
		}
		else {
			/* Unknown trap was encountered... */
			return 0;
		}
	}
	/* Unknown status... */
	return 0;
}

long
gimme_arg(enum tof type, struct process * proc, int arg_num) {
	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR2, 0);
	}

	if (type==LT_TOF_FUNCTION) {
		switch(arg_num) {
			case 0: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_ORIGGPR2, 0);
			case 1: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR3, 0);
			case 2: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR4, 0);
			case 3: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR5, 0);
			case 4: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR6, 0);
			default:
					fprintf(stderr, "gimme_arg called with wrong arguments\n");
					exit(2);
		}

	} else if (type==LT_TOF_SYSCALL) {
		switch(arg_num) {
			case 0: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_ORIGGPR2, 0);
			case 1: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR3, 0);
			case 2: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR4, 0);
			case 3: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR5, 0);
			case 4: return ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR6, 0);
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
