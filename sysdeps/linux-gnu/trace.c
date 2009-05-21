#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ptrace.h"
#include <asm/unistd.h>

#include "ltrace.h"
#include "options.h"
#include "sysdep.h"
#include "debug.h"

/* If the system headers did not provide the constants, hard-code the normal
   values.  */
#ifndef PTRACE_EVENT_FORK

#define PTRACE_OLDSETOPTIONS    21
#define PTRACE_SETOPTIONS       0x4200
#define PTRACE_GETEVENTMSG      0x4201

/* options set using PTRACE_SETOPTIONS */
#define PTRACE_O_TRACESYSGOOD   0x00000001
#define PTRACE_O_TRACEFORK      0x00000002
#define PTRACE_O_TRACEVFORK     0x00000004
#define PTRACE_O_TRACECLONE     0x00000008
#define PTRACE_O_TRACEEXEC      0x00000010
#define PTRACE_O_TRACEVFORKDONE 0x00000020
#define PTRACE_O_TRACEEXIT      0x00000040

/* Wait extended result codes for the above trace options.  */
#define PTRACE_EVENT_FORK       1
#define PTRACE_EVENT_VFORK      2
#define PTRACE_EVENT_CLONE      3
#define PTRACE_EVENT_EXEC       4
#define PTRACE_EVENT_VFORK_DONE 5
#define PTRACE_EVENT_EXIT       6

#endif /* PTRACE_EVENT_FORK */

static int fork_exec_syscalls[][5] = {
	{
#ifdef __NR_fork
	 __NR_fork,
#else
	 -1,
#endif
#ifdef __NR_clone
	 __NR_clone,
#else
	 -1,
#endif
#ifdef __NR_clone2
	 __NR_clone2,
#else
	 -1,
#endif
#ifdef __NR_vfork
	 __NR_vfork,
#else
	 -1,
#endif
#ifdef __NR_execve
	 __NR_execve,
#else
	 -1,
#endif
	 }
#ifdef FORK_EXEC_SYSCALLS
	FORK_EXEC_SYSCALLS
#endif
};

#ifdef ARCH_HAVE_UMOVELONG
extern int arch_umovelong (Process *, void *, long *, arg_type_info *);
int
umovelong (Process *proc, void *addr, long *result, arg_type_info *info) {
	return arch_umovelong (proc, addr, result, info);
}
#else
/* Read a single long from the process's memory address 'addr' */
int
umovelong (Process *proc, void *addr, long *result, arg_type_info *info) {
	long pointed_to;

	errno = 0;
	pointed_to = ptrace (PTRACE_PEEKTEXT, proc->pid, addr, 0);
	if (pointed_to == -1 && errno)
		return -errno;

	*result = pointed_to;
	return 0;
}
#endif

void
trace_me(void) {
	debug(DEBUG_PROCESS, "trace_me: pid=%d\n", getpid());
	if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
		perror("PTRACE_TRACEME");
		exit(1);
	}
}

int
trace_pid(pid_t pid) {
	debug(DEBUG_PROCESS, "trace_pid: pid=%d\n", pid);
	if (ptrace(PTRACE_ATTACH, pid, 1, 0) < 0) {
		return -1;
	}

	/* man ptrace: PTRACE_ATTACH attaches to the process specified
	   in pid.  The child is sent a SIGSTOP, but will not
	   necessarily have stopped by the completion of this call;
	   use wait() to wait for the child to stop. */
	if (waitpid (pid, NULL, 0) != pid) {
		perror ("trace_pid: waitpid");
		exit (1);
	}

	return 0;
}

void
trace_set_options(Process *proc, pid_t pid) {
	if (proc->tracesysgood & 0x80)
		return;

	debug(DEBUG_PROCESS, "trace_set_options: pid=%d\n", pid);

	long options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
		PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE |
		PTRACE_O_TRACEEXEC;
	if (ptrace(PTRACE_SETOPTIONS, pid, 0, options) < 0 &&
	    ptrace(PTRACE_OLDSETOPTIONS, pid, 0, options) < 0) {
		perror("PTRACE_SETOPTIONS");
		return;
	}
	proc->tracesysgood |= 0x80;
}

void
untrace_pid(pid_t pid) {
	debug(DEBUG_PROCESS, "untrace_pid: pid=%d\n", pid);
	ptrace(PTRACE_DETACH, pid, 1, 0);
}

void
continue_after_signal(pid_t pid, int signum) {
	Process *proc;

	debug(DEBUG_PROCESS, "continue_after_signal: pid=%d, signum=%d", pid, signum);

	proc = pid2proc(pid);
	if (proc && proc->breakpoint_being_enabled) {
#if defined __sparc__  || defined __ia64___
		ptrace(PTRACE_SYSCALL, pid, 0, signum);
#else
		ptrace(PTRACE_SINGLESTEP, pid, 0, signum);
#endif
	} else {
		ptrace(PTRACE_SYSCALL, pid, 0, signum);
	}
}

void
continue_process(pid_t pid) {
	/* We always trace syscalls to control fork(), clone(), execve()... */

	debug(DEBUG_PROCESS, "continue_process: pid=%d", pid);

	ptrace(PTRACE_SYSCALL, pid, 0, 0);
}

void
continue_enabling_breakpoint(pid_t pid, Breakpoint *sbp) {
	enable_breakpoint(pid, sbp);
	continue_process(pid);
}

void
continue_after_breakpoint(Process *proc, Breakpoint *sbp) {
	if (sbp->enabled)
		disable_breakpoint(proc->pid, sbp);
	set_instruction_pointer(proc, sbp->addr);
	if (sbp->enabled == 0) {
		continue_process(proc->pid);
	} else {
		debug(DEBUG_PROCESS, "continue_after_breakpoint: pid=%d, addr=%p", proc->pid, sbp->addr);
		proc->breakpoint_being_enabled = sbp;
#if defined __sparc__  || defined __ia64___
		/* we don't want to singlestep here */
		continue_process(proc->pid);
#else
		ptrace(PTRACE_SINGLESTEP, proc->pid, 0, 0);
#endif
	}
}

/* Read a series of bytes starting at the process's memory address
   'addr' and continuing until a NUL ('\0') is seen or 'len' bytes
   have been read.
*/
int
umovestr(Process *proc, void *addr, int len, void *laddr) {
	union {
		long a;
		char c[sizeof(long)];
	} a;
	int i;
	int offset = 0;

	while (offset < len) {
		a.a = ptrace(PTRACE_PEEKTEXT, proc->pid, addr + offset, 0);
		for (i = 0; i < sizeof(long); i++) {
			if (a.c[i] && offset + (signed)i < len) {
				*(char *)(laddr + offset + i) = a.c[i];
			} else {
				*(char *)(laddr + offset + i) = '\0';
				return 0;
			}
		}
		offset += sizeof(long);
	}
	*(char *)(laddr + offset) = '\0';
	return 0;
}
