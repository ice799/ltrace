#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "ltrace.h"
#include "process.h"
#include "symbols.h"
#include "functions.h"
#include "i386.h"

struct process * list_of_processes = NULL;

int attach_process(const char * file, char * const argv[])
{
	int status;
	int pid = fork();
	struct process * tmp;

	if (pid<0) {
		perror("fork");
		exit(1);
	} else if (!pid) {
		trace_me();
		execvp(file, argv);
		fprintf(stderr, "Can't execute \"%s\": %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}
	/* Wait until execve... */
	if (wait4(pid, &status, 0, NULL)==-1) {
		perror("wait4");
		exit(1);
	}
	if (!(WIFSTOPPED(status) && (WSTOPSIG(status)==SIGTRAP))) {
		fprintf(stderr, "Unknown exit status for process %s\n", file);
		exit(1);
	}
	tmp = (struct process *)malloc(sizeof(struct process));
	tmp->pid = pid;
	tmp->next = list_of_processes;
	list_of_processes = tmp;
	
	return pid;
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
	unsigned int orig_eax;

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
fprintf(stderr, "errno=%d\n", errno);
		if (errno == ECHILD) {
			fprintf(output, "No more children\n");
			exit(0);
		}
		perror("wait4");
		exit(1);
	}
	if (WIFEXITED(status)) {
		fprintf(output, "pid %u exited\n", pid);
		return;
	}
	if (WIFSIGNALED(status)) {
		fprintf(output, "pid %u exited on signal %u\n", pid, WTERMSIG(status));
		return;
	}
	if (!WIFSTOPPED(status)) {
		fprintf(output, "pid %u ???\n", pid);
		return;
	}
	fprintf(output, "<pid:%u> ", pid);
	eip = get_eip(pid);
orig_eax = get_orig_eax(pid);
if (orig_eax!=-1) {
fprintf(output,"[0x%08lx] SYSCALL: %u\n", eip, orig_eax);
continue_process(pid, 0);
return;
}
	if (WSTOPSIG(status) != SIGTRAP) {
		fprintf(output, "[0x%08lx] Signal: %u\n", eip, WSTOPSIG(status));
		continue_process(pid, WSTOPSIG(status));
		return;
	}
	/* pid is stopped... */
	esp = get_esp(pid);
#if 0
	fprintf(output,"EIP = 0x%08x\n", eip);
	fprintf(output,"ESP = 0x%08x\n", esp);
#endif
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
