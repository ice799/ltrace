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

char * command;
struct process * list_of_processes = NULL;

static struct process * open_program(void);

int main(int argc, char **argv)
{
	argv = process_options(argc, argv);
	if (opt_p || opt_f) {
		fprintf(stderr, "ERROR: Options -p and -f don't work yet\n");
		exit(1);
	}
	read_config_file("/etc/ltrace.conf");
	if (getenv("HOME")) {
		char path[PATH_MAX];
		sprintf(path, getenv("HOME"));	/* FIXME: buffer overrun */
		strcat(path, "/.ltrace.conf");
		read_config_file(path);
	}
	execute_program(open_program(), argv);
	while(1) {
		process_event(wait_for_something());
	}
}

static struct process * open_program(void)
{
	list_of_processes = malloc(sizeof(struct process));
	if (!list_of_processes) {
		perror("malloc");
		exit(1);
	}
	list_of_processes->filename = command;
	list_of_processes->pid = 0;
	list_of_processes->breakpoints_enabled = -1;
	list_of_processes->current_syscall = -1;
	list_of_processes->current_symbol = NULL;
	list_of_processes->breakpoint_being_enabled = NULL;
	list_of_processes->next = NULL;
	if (opt_L) {
		list_of_processes->list_of_symbols = read_elf(command);
	} else {
		list_of_processes->list_of_symbols = NULL;
	}

	return list_of_processes;
}
