#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>

#include "ltrace.h"
#include "elf.h"
#include "output.h"
#include "config_file.h"
#include "options.h"

char * command = NULL;
struct process * list_of_processes = NULL;

static void normal_exit(void);
static void signal_exit(int);

int main(int argc, char **argv)
{
	atexit(normal_exit);
	signal(SIGINT,signal_exit);	/* Detach processes when interrupted */
	signal(SIGTERM,signal_exit);	/*  ... or killed */

	argv = process_options(argc, argv);
	read_config_file("/etc/ltrace.conf");
	if (getenv("HOME")) {
		char path[PATH_MAX];
		sprintf(path, getenv("HOME"));	/* FIXME: buffer overrun */
		strcat(path, "/.ltrace.conf");
		read_config_file(path);
	}
	while (opt_p) {
		open_pid(opt_p->pid, 1);
		opt_p = opt_p->next;
	}
	if (command) {
		execute_program(open_program(command), argv);
	}
	while(1) {
		process_event(wait_for_something());
	}
}

static void signal_exit(int sig)
{
	exit(2);
}

static void normal_exit(void)
{
	struct process * tmp = list_of_processes;

	while(tmp) {
		kill(tmp->pid, SIGSTOP);
		disable_all_breakpoints(tmp);
		continue_process(tmp->pid);
		untrace_pid(tmp->pid);
		tmp = tmp->next;
	}
}
