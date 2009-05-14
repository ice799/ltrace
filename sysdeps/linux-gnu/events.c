#if HAVE_CONFIG_H
#include "config.h"
#endif

#define	_GNU_SOURCE	1
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"
#include "debug.h"

static Event event;

Event *
next_event(void) {
	pid_t pid;
	int status;
	int tmp;
	int stop_signal;

	debug(DEBUG_FUNCTION, "next_event()");
	if (!list_of_processes) {
		debug(DEBUG_EVENT, "event: No more traced programs: exiting");
		exit(0);
	}
	pid = wait(&status);
	if (pid == -1) {
		if (errno == ECHILD) {
			debug(DEBUG_EVENT, "event: No more traced programs: exiting");
			exit(0);
		} else if (errno == EINTR) {
			debug(DEBUG_EVENT, "event: none (wait received EINTR?)");
			event.type = EVENT_NONE;
			return &event;
		}
		perror("wait");
		exit(1);
	}
	event.proc = pid2proc(pid);
	if (!event.proc) {
		event.type = EVENT_NEW;
		debug(DEBUG_EVENT, "event: NEW: pid=%d", pid);
		return &event;
	}
	get_arch_dep(event.proc);
	event.proc->instruction_pointer = NULL;
	debug(3, "event from pid %u", pid);
	if (event.proc->breakpoints_enabled == -1) {
		enable_all_breakpoints(event.proc);
		event.type = EVENT_NONE;
		trace_set_options(event.proc, event.proc->pid);
		continue_process(event.proc->pid);
		debug(DEBUG_EVENT, "event: NONE: pid=%d (enabling breakpoints)", pid);
		return &event;
	}
	if (opt_i) {
		event.proc->instruction_pointer =
		    get_instruction_pointer(event.proc);
	}
	switch (syscall_p(event.proc, status, &tmp)) {
		case 1:
			event.type = EVENT_SYSCALL;
			event.e_un.sysnum = tmp;
			debug(DEBUG_EVENT, "event: SYSCALL: pid=%d, sysnum=%d", pid, tmp);
			return &event;
		case 2:
			event.type = EVENT_SYSRET;
			event.e_un.sysnum = tmp;
			debug(DEBUG_EVENT, "event: SYSRET: pid=%d, sysnum=%d", pid, tmp);
			return &event;
		case 3:
			event.type = EVENT_ARCH_SYSCALL;
			event.e_un.sysnum = tmp;
			debug(DEBUG_EVENT, "event: ARCH_SYSCALL: pid=%d, sysnum=%d", pid, tmp);
			return &event;
		case 4:
			event.type = EVENT_ARCH_SYSRET;
			event.e_un.sysnum = tmp;
			debug(DEBUG_EVENT, "event: ARCH_SYSRET: pid=%d, sysnum=%d", pid, tmp);
			return &event;
		case -1:
			event.type = EVENT_NONE;
			continue_process(event.proc->pid);
			debug(DEBUG_EVENT, "event: NONE: pid=%d (syscall_p returned -1)", pid);
			return &event;
	}
	if (WIFSTOPPED(status) && ((status>>16 == PTRACE_EVENT_FORK) || (status>>16 == PTRACE_EVENT_VFORK) || (status>>16 == PTRACE_EVENT_CLONE))) {
		unsigned long data;
		ptrace(PTRACE_GETEVENTMSG, pid, NULL, &data);
		event.type = EVENT_CLONE;
		event.e_un.newpid = data;
		debug(DEBUG_EVENT, "event: CLONE: pid=%d, newpid=%d", pid, (int)data);
		return &event;
	}
	if (WIFSTOPPED(status) && (status>>16 == PTRACE_EVENT_EXEC)) {
		event.type = EVENT_EXEC;
		debug(DEBUG_EVENT, "event: EXEC: pid=%d", pid);
		return &event;
	}
	if (WIFEXITED(status)) {
		event.type = EVENT_EXIT;
		event.e_un.ret_val = WEXITSTATUS(status);
		debug(DEBUG_EVENT, "event: EXIT: pid=%d, status=%d", pid, event.e_un.ret_val);
		return &event;
	}
	if (WIFSIGNALED(status)) {
		event.type = EVENT_EXIT_SIGNAL;
		event.e_un.signum = WTERMSIG(status);
		debug(DEBUG_EVENT, "event: EXIT_SIGNAL: pid=%d, signum=%d", pid, event.e_un.signum);
		return &event;
	}
	if (!WIFSTOPPED(status)) {
		event.type = EVENT_NONE;
		debug(DEBUG_EVENT, "event: NONE: pid=%d (wait error?)", pid);
		return &event;
	}

	stop_signal = WSTOPSIG(status);

	/* On some targets, breakpoints are signalled not using
	   SIGTRAP, but also with SIGILL, SIGSEGV or SIGEMT.  Check
	   for these. */
	if (stop_signal == SIGSEGV
	    || stop_signal == SIGILL
#ifdef SIGEMT
	    || stop_signal == SIGEMT
#endif
	    ) {
		// If we didn't need to know IP so far, get it now.
		void * addr = opt_i
		  ? event.proc->instruction_pointer
		  : (event.proc->instruction_pointer = get_instruction_pointer (event.proc));

		if (address2bpstruct(event.proc, addr))
			stop_signal = SIGTRAP;
	}

	if (stop_signal != (SIGTRAP | event.proc->tracesysgood)
	    && stop_signal != SIGTRAP) {
		event.type = EVENT_SIGNAL;
		event.e_un.signum = stop_signal;
		debug(DEBUG_EVENT, "event: SIGNAL: pid=%d, signum=%d", pid, stop_signal);
		return &event;
	}

	if (was_exec(event.proc, status)) {
		pid_t saved_pid;

		event.type = EVENT_NONE;
		event.e_un.signum = WSTOPSIG(status);
		debug(1, "Placing breakpoints for the new program");
		event.proc->mask_32bit = 0;
		event.proc->personality = 0;
		event.proc->arch_ptr = NULL;
		event.proc->filename = pid2name(event.proc->pid);
		saved_pid = event.proc->pid;
		event.proc->pid = 0;
		breakpoints_init(event.proc);
		event.proc->pid = saved_pid;
		continue_process(event.proc->pid);
		debug(DEBUG_EVENT, "event: NONE: pid=%d (was_exec; placed breakpoints)", pid);
		return &event;
	}

	event.type = EVENT_BREAKPOINT;
	if (!event.proc->instruction_pointer) {
		event.proc->instruction_pointer =
		    get_instruction_pointer(event.proc);
	}
	event.e_un.brk_addr =
	    event.proc->instruction_pointer - DECR_PC_AFTER_BREAK;
	debug(DEBUG_EVENT, "event: BREAKPOINT: pid=%d, addr=%p", pid, event.e_un.brk_addr);
	return &event;
}
