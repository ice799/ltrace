#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>

#include "ltrace.h"
#include "elf.h"
#include "output.h"
#include "config_file.h"
#include "options.h"

char * command = NULL;
struct process * list_of_processes = NULL;

static struct process * open_program(char * filename);
static void open_pid(pid_t pid);

int main(int argc, char **argv)
{
	argv = process_options(argc, argv);
	read_config_file("/etc/ltrace.conf");
	if (getenv("HOME")) {
		char path[PATH_MAX];
		sprintf(path, getenv("HOME"));	/* FIXME: buffer overrun */
		strcat(path, "/.ltrace.conf");
		read_config_file(path);
	}
	while (opt_p) {
		open_pid(opt_p->pid);
		opt_p = opt_p->next;
	}
	if (opt_f) {
		fprintf(stderr, "ERROR: Option -f doesn't work yet\n");
		exit(1);
	}
	if (command) {
		execute_program(open_program(command), argv);
	}
	while(1) {
		process_event(wait_for_something());
	}
}

static struct process * open_program(char * filename)
{
	struct process * proc;
	proc = malloc(sizeof(struct process));
	if (!proc) {
		perror("malloc");
		exit(1);
	}
	proc->filename = filename;
	proc->pid = 0;
	proc->breakpoints_enabled = -1;
	proc->current_syscall = -1;
	proc->current_symbol = NULL;
	proc->breakpoint_being_enabled = NULL;
	proc->next = NULL;
	if (opt_L) {
		proc->list_of_symbols = read_elf(filename);
	} else {
		proc->list_of_symbols = NULL;
	}

	proc->next = list_of_processes;
	list_of_processes = proc;
	return proc;
}

static void open_pid(pid_t pid)
{
	struct process * proc;
	char * filename = pid2name(pid);

	if (!filename) {
		fprintf(stderr, "Cannot trace pid %u: %s\n", pid, strerror(errno));
		return;
	}

	trace_pid(pid);
	proc = open_program(filename);
	proc->pid = pid;
}
