#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "ltrace.h"
#include "options.h"
#include "elf.h"

Process *
open_program(char *filename, pid_t pid) {
	Process *proc;
	proc = calloc(sizeof(Process), 1);
	if (!proc) {
		perror("malloc");
		exit(1);
	}
	proc->filename = strdup(filename);
	proc->breakpoints_enabled = -1;
	if (pid) {
		proc->pid = pid;
	}
	breakpoints_init(proc);

	proc->next = list_of_processes;
	list_of_processes = proc;
	return proc;
}

void
open_pid(pid_t pid, int verbose) {
	Process *proc;
	char *filename;

	if (trace_pid(pid) < 0) {
		fprintf(stderr, "Cannot attach to pid %u: %s\n", pid,
			strerror(errno));
		return;
	}

	filename = pid2name(pid);

#if 0
	if (!filename) {
		if (verbose) {
			fprintf(stderr, "Cannot trace pid %u: %s\n", pid,
				strerror(errno));
		}
		return;
	}
#endif

	proc = open_program(filename, pid);
	continue_process(pid);
	proc->breakpoints_enabled = 1;
}

Process *
pid2proc(pid_t pid) {
	Process *tmp;

	tmp = list_of_processes;
	while (tmp) {
		if (pid == tmp->pid) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}
