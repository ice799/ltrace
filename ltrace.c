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

