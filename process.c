#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <asm/unistd.h>

#include "i386.h"
#include "ltrace.h"
#include "process.h"
#include "symbols.h"
#include "functions.h"
#include "syscall.h"
#include "signal.h"
#include "output.h"

struct process * list_of_processes = NULL;

unsigned int instruction_pointer;

static void detach_process(int pid);
static struct process * pid2proc(int pid);
static void process_child(struct process * current_process);

int execute_process(const char * file, char * const argv[])
{
	struct process * tmp;
	int pid = fork();

	if (pid<0) {
		perror("fork");
		exit(1);
	} else if (!pid) {
		trace_me();
		execvp(file, argv);
		fprintf(stderr, "Can't execute \"%s\": %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}

	tmp = (struct process *)malloc(sizeof(struct process));
	tmp->pid = pid;
	tmp->breakpoints_enabled = 0;
	tmp->syscall_number = -1;
	tmp->next = list_of_processes;
	list_of_processes = tmp;
	
	return pid;
}

static void detach_process(int pid)
{
	struct process *tmp, *tmp2;

	if (list_of_processes && (pid == list_of_processes->pid)) {
		tmp2 = list_of_processes->next;
		free(list_of_processes);
		list_of_processes = tmp2;
	} else {
		tmp = list_of_processes;
		while(tmp && tmp->next) {
			if (pid == tmp->next->pid) {
				tmp2 = tmp->next->next;
				free(tmp->next);
				tmp->next = tmp2;
			}
			tmp = tmp->next;
		}
	}
	if (!kill(pid,0)) {
		disable_all_breakpoints(pid);
		untrace_pid(pid);
	}
}

static struct process * pid2proc(int pid)
{
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

void wait_for_child(void)
{
	int pid;
	int status;
	struct process * current_process;

	pid = wait4(-1, &status, 0, NULL);
	if (pid==-1) {
		if (errno == ECHILD) {
			send_line("No more children");
			exit(0);
		}
		perror("wait4");
		exit(1);
	}
	current_process = pid2proc(pid);
	if (!current_process) {
		fprintf(stderr, "wrong pid %d ???\n", pid);
		exit(1);
	}
	if (!current_process->breakpoints_enabled) {
		if (opt_d>0) {
			send_line("Enabling breakpoints for pid %d...", pid);
		}
		enable_all_breakpoints(pid);
		current_process->breakpoints_enabled=1;
	}
	if (WIFEXITED(status)) {
		send_line("pid %u exited", pid);
		detach_process(pid);
		return;
	}
	if (WIFSIGNALED(status)) {
		send_line("--- %s (%s) ---", signal_name[WSTOPSIG(status)], strsignal(WSTOPSIG(status)));
		send_line("+++ killed by %s +++", signal_name[WSTOPSIG(status)]);
		detach_process(pid);
		return;
	}
	if (!WIFSTOPPED(status)) {
		send_line("pid %u ???", pid);
		exit(1);
	}
	if (WSTOPSIG(status) != SIGTRAP) {
		send_line("--- %s (%s) ---", signal_name[WSTOPSIG(status)], strsignal(WSTOPSIG(status)));
		continue_process(pid, WSTOPSIG(status));
		return;
	}
	process_child(current_process);
}

static void process_child(struct process * current_process)
{
	int pid;
	unsigned long eip;
	int esp;
	int function_seen;
	int status;
	struct library_symbol * tmp = NULL;

	pid = current_process->pid;
	eip = get_eip(pid);
	instruction_pointer = eip;

	switch (type_of_stop(current_process, &status)) {
		case PROC_SYSCALL:
			if (status==__NR_fork) {
				disable_all_breakpoints(pid);
			}
			if (opt_S) {
				send_line("SYSCALL: %s()", syscall_list[status]);
			}
			continue_process(pid, 0);
			return;
		case PROC_SYSRET:
			if (status==__NR_fork) {
				enable_all_breakpoints(pid);
			}
			if (opt_S && (opt_d>0)) {
				send_line("SYSRET:  %u", status);
			}
			continue_process(pid, 0);
			return;
		case PROC_BREAKPOINT:
		default:
	}
	/* pid is breakpointed... */
	/* TODO: I may be here after a PTRACE_SINGLESTEP ... */
	esp = get_esp(pid);
	instruction_pointer = get_return(pid, esp);
	tmp = library_symbols;
	function_seen = 0;
	while(tmp) {
		if (eip == tmp->addr) {
			function_seen = 1;
			print_function(tmp->name, pid, esp);
			continue_after_breakpoint(pid, eip, tmp->old_value, 0);
			break;
		}
		tmp = tmp->next;
	}
	if (!function_seen) {
		send_line("pid %u stopped; continuing it...", pid);
		continue_process(pid, 0);
	}
}
