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

#include "ltrace.h"
#include "options.h"
#include "debug.h"

static struct event event;

/* This should also update `current_process' */

static struct process * pid2proc(int pid);

struct event *
wait_for_something(void) {
	pid_t pid;
	int status;
	int tmp;

	if (!list_of_processes) {
		debug(1, "No more children");
		exit(0);
	}
	pid = wait(&status);
	if (pid==-1) {
		if (errno==ECHILD) {
			debug(1, "No more children");
			exit(0);
		} else if (errno==EINTR) {
			debug(1, "wait received EINTR ?");
			event.thing = LT_EV_NONE;
			return &event;
		}
		perror("wait");
		exit(1);
	}
	event.proc = pid2proc(pid);
	if (!event.proc) {
		fprintf(stderr, "signal from wrong pid %u ?!?\n", pid);
		exit(1);
	}
	get_arch_dep(event.proc);
	event.proc->instruction_pointer = NULL;
	debug(3, "signal from pid %u", pid);
	if (event.proc->breakpoints_enabled == -1) {
		enable_all_breakpoints(event.proc);
		event.thing = LT_EV_NONE;
		trace_set_options(event.proc, event.proc->pid);
		continue_process(event.proc->pid);
		return &event;
	}
	if (opt_i) {
		event.proc->instruction_pointer = get_instruction_pointer(event.proc);
	}
	switch(syscall_p(event.proc, status, &tmp)) {
		case 1:	event.thing = LT_EV_SYSCALL;
			event.e_un.sysnum = tmp;
			return &event;
		case 2:	event.thing = LT_EV_SYSRET;
			event.e_un.sysnum = tmp;
			return &event;
	}
	if (WIFEXITED(status)) {
		event.thing = LT_EV_EXIT;
		event.e_un.ret_val = WEXITSTATUS(status);
		return &event;
	}
	if (WIFSIGNALED(status)) {
		event.thing = LT_EV_EXIT_SIGNAL;
		event.e_un.signum = WTERMSIG(status);
		return &event;
	}
	if (!WIFSTOPPED(status)) {
		event.thing = LT_EV_UNKNOWN;
		return &event;
	}
	if ((WSTOPSIG(status) != (SIGTRAP | event.proc->tracesysgood)) &&
	    (WSTOPSIG(status) != SIGTRAP)) {
		event.thing = LT_EV_SIGNAL;
		event.e_un.signum = WSTOPSIG(status);
		return &event;
	}
	event.thing = LT_EV_BREAKPOINT;
	if (!event.proc->instruction_pointer) {
		event.proc->instruction_pointer = get_instruction_pointer(event.proc);
	}
	event.e_un.brk_addr = event.proc->instruction_pointer - DECR_PC_AFTER_BREAK;
	return &event;
}

static struct process *
pid2proc(pid_t pid) {
	struct process * tmp;

	tmp = list_of_processes;
	while(tmp) {
		if (pid == tmp->pid) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}
