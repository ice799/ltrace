#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ltrace.h"
#include "output.h"
#include "options.h"
#include "elf.h"

static void process_signal(struct event * event);
static void process_exit(struct event * event);
static void process_exit_signal(struct event * event);
static void process_syscall(struct event * event);
static void process_sysret(struct event * event);
static void process_breakpoint(struct event * event);

void process_event(struct event * event)
{
	switch (event->thing) {
		case LT_EV_NONE:
			return;
		case LT_EV_SIGNAL:
			if (opt_d>0) {
				output_line(0, "event: signal (%d)", event->e_un.signum);
			}
			process_signal(event);
			return;
		case LT_EV_EXIT:
			if (opt_d>0) {
				output_line(0, "event: exit (%d)", event->e_un.ret_val);
			}
			process_exit(event);
			return;
		case LT_EV_EXIT_SIGNAL:
			if (opt_d>0) {
				output_line(0, "event: exit signal (%d)", event->e_un.signum);
			}
			process_exit_signal(event);
			return;
		case LT_EV_SYSCALL:
			if (opt_d>0) {
				output_line(0, "event: syscall (%d)", event->e_un.sysnum);
			}
			process_syscall(event);
			return;
		case LT_EV_SYSRET:
			if (opt_d>0) {
				output_line(0, "event: sysret (%d)", event->e_un.sysnum);
			}
			process_sysret(event);
			return;
		case LT_EV_BREAKPOINT:
			process_breakpoint(event);
			return;
		default:
			fprintf(stderr, "Error! unknown event?\n");
			exit(1);
	}
}

static char * shortsignal(int signum)
{
	static char * signalent0[] = {
	#include "signalent.h"
	};
	int nsignals0 = sizeof signalent0 / sizeof signalent0[0];

	if (signum<0 || signum>nsignals0) {
		return "UNKNOWN_SIGNAL";
	} else {
		return signalent0[signum];
	}
}

static char * sysname(int sysnum)
{
	static char result[128];
	static char * syscalent0[] = {
	#include "syscallent.h"
	};
	int nsyscals0 = sizeof syscalent0 / sizeof syscalent0[0];

	if (sysnum<0 || sysnum>nsyscals0) {
		sprintf(result, "SYS_%d", sysnum);
		return result;
	} else {
		sprintf(result, "SYS_%s", syscalent0[sysnum]);
		return result;
	}
}

static void process_signal(struct event * event)
{
	output_line(event->proc, "--- %s (%s) ---",
		shortsignal(event->e_un.signum), strsignal(event->e_un.signum));
	continue_after_signal(event->proc->pid, event->e_un.signum);
}

static void remove_proc(struct process * proc);

static void process_exit(struct event * event)
{
	output_line(event->proc, "+++ exited (status %d) +++",
		event->e_un.ret_val);
	remove_proc(event->proc);
}

static void process_exit_signal(struct event * event)
{
	output_line(event->proc, "+++ killed by %s +++",
		shortsignal(event->e_un.signum));
	remove_proc(event->proc);
}

static void remove_proc(struct process * proc)
{
	struct process *tmp, *tmp2;

	if (list_of_processes == proc) {
		tmp = list_of_processes;
		list_of_processes = list_of_processes->next;
		free(tmp);
		return;
	}
	tmp = list_of_processes;
	while(tmp->next) {
		if (tmp->next==proc) {
			tmp2 = tmp->next;
			tmp->next = tmp->next->next;
			free(tmp2);
		}
		tmp = tmp->next;
	}
}

static void process_syscall(struct event * event)
{
	event->proc->current_syscall = event->e_un.sysnum;
	if (opt_S) {
		output_left(LT_TOF_SYSCALL, event->proc, sysname(event->e_un.sysnum));
	}
	if (fork_p(event->e_un.sysnum) || exec_p(event->e_un.sysnum)) {
		disable_all_breakpoints(event->proc);
	} else if (!event->proc->breakpoints_enabled) {
		enable_all_breakpoints(event->proc);
	}
	continue_process(event->proc->pid);
}

static void process_sysret(struct event * event)
{
	if (opt_S) {
		output_right(LT_TOF_SYSCALL, event->proc, sysname(event->e_un.sysnum));
	}
	if (exec_p(event->e_un.sysnum)) {
		if (gimme_arg(LT_TOF_SYSCALL,event->proc,-1)==0) {
			event->proc->filename = pid2name(event->proc->pid);
			event->proc->list_of_symbols = read_elf(event->proc->filename);
			event->proc->breakpoints_enabled = -1;
		} else {
			enable_all_breakpoints(event->proc);
		}
	}
	if (fork_p(event->e_un.sysnum)) {
		if (opt_f) {
			pid_t child = gimme_arg(LT_TOF_SYSCALL,event->proc,-1);
			if (child>0) {
				open_pid(child, 0);
			}
		}
		enable_all_breakpoints(event->proc);
	}
	event->proc->current_syscall = -1;
	continue_process(event->proc->pid);
}

static void process_breakpoint(struct event * event)
{
	struct library_symbol * tmp;

	if (opt_d>1) {
		output_line(0,"event: breakpoint (0x%08x)", event->e_un.brk_addr);
	}
	if (event->proc->breakpoint_being_enabled) {
		continue_enabling_breakpoint(event->proc->pid, event->proc->breakpoint_being_enabled);
		event->proc->breakpoint_being_enabled = NULL;
		return;
	}
	if (event->proc->current_symbol && event->e_un.brk_addr == event->proc->return_value.addr) {
		output_right(LT_TOF_FUNCTION, event->proc, event->proc->current_symbol->name);
		continue_after_breakpoint(event->proc, &event->proc->return_value, 1);
		event->proc->current_symbol = NULL;
		return;
	}

	tmp = event->proc->list_of_symbols;
	while(tmp) {
		if (event->e_un.brk_addr == tmp->brk.addr) {
			if (event->proc->current_symbol) {
				delete_breakpoint(event->proc->pid, &event->proc->return_value);
			}
			event->proc->current_symbol = tmp;
			event->proc->stack_pointer = get_stack_pointer(event->proc->pid);
			event->proc->return_addr = get_return_addr(event->proc->pid, event->proc->stack_pointer);
			output_left(LT_TOF_FUNCTION, event->proc, tmp->name);
			event->proc->return_value.addr = event->proc->return_addr;
			insert_breakpoint(event->proc->pid, &event->proc->return_value);
			continue_after_breakpoint(event->proc, &tmp->brk, 0);
			return;
		}
		tmp = tmp->next;
	}
	output_line(event->proc, "breakpointed at 0x%08x (?)",
		(unsigned)event->e_un.brk_addr);
	continue_process(event->proc->pid);
}
