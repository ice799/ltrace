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

struct process * list_of_processes = NULL;

int execute_process(const char * file, char * const argv[])
{
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
	
	return pid;
}

struct process * attach_process(int pid)
{
	struct process * tmp;

	tmp = (struct process *)malloc(sizeof(struct process));
	tmp->pid = pid;
	tmp->syscall_number = -1;
	tmp->next = list_of_processes;
	list_of_processes = tmp;

	return tmp;
}

/*
 * TODO: All the breakpoints should be disabled.
 */
void detach_process(int pid)
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
		untrace_pid(pid);
	}
}

struct process * pid2proc(int pid)
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

static void chld_handler();
void init_sighandler(void)
{
	signal(SIGCHLD, chld_handler);
}

static void chld_handler()
{
	int pid;
	unsigned long eip;
	int esp;
	int function_seen;
	int status;
	struct library_symbol * tmp = NULL;
	struct process * current_process;

	signal(SIGCHLD, chld_handler);

	pid = wait4(-1, &status, WNOHANG, NULL);
	if (!pid) {
		if (errno!=0) {
			perror("wait4");
			exit(1);
		}
		return;
	}
	if (pid==-1) {
		perror("wait4");
		exit(1);
	}
	current_process = pid2proc(pid);
	if (!current_process) {
		/*
 		* I should assume here that this process is new, so I
		* have to enable breakpoints
 		*/
		if (opt_d>0) {
			fprintf(output, "new process %d. Attaching it...\n", pid);
		}
		current_process = attach_process(pid);
		if (opt_d>0) {
			fprintf(output, "Enabling breakpoints for pid %d...\n", pid);
		}
		enable_all_breakpoints(pid);
	}
	if (WIFEXITED(status)) {
		fprintf(output, "pid %u exited\n", pid);
		detach_process(pid);
		return;
	}
	if (WIFSIGNALED(status)) {
		fprintf(output, "pid %u exited on signal %u\n", pid, WTERMSIG(status));
		detach_process(pid);
		return;
	}
	if (!WIFSTOPPED(status)) {
		fprintf(output, "pid %u ???\n", pid);
		exit(1);
	}
	eip = get_eip(pid);
	if (WSTOPSIG(status) != SIGTRAP) {
		fprintf(output, "[0x%08lx] Signal: %u\n", eip, WSTOPSIG(status));
		continue_process(pid, WSTOPSIG(status));
		return;
	}
	if (opt_d>0) {
		fprintf(output, "<pid:%u> ", pid);
	}
	switch (type_of_stop(current_process, &status)) {
		case PROC_SYSCALL:
			if (status==__NR_fork) {
				disable_all_breakpoints(pid);
			}
			fprintf(output, "[0x%08lx] SYSCALL: %s()\n", eip, syscall_list[status]);
			continue_process(pid, 0);
			return;
		case PROC_SYSRET:
			if (status==__NR_fork) {
				enable_all_breakpoints(pid);
			}
			if (opt_d>0) {
				fprintf(output, "[0x%08lx] SYSRET:  %u\n", eip, status);
			}
			continue_process(pid, 0);
			return;
		case PROC_BREAKPOINT:
		default:
	}
	/* pid is breakpointed... */
	/* TODO: I may be here after a PTRACE_SINGLESTEP ... */
	esp = get_esp(pid);
	fprintf(output,"[0x%08lx] ", get_return(pid,esp));
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
		fprintf(output, "pid %u stopped; continuing it...\n", pid);
		continue_process(pid, 0);
	}
}
