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

struct process *
open_program(char * filename) {
	struct process * proc;
	struct library_symbol * sym;
	proc = malloc(sizeof(struct process));
	if (!proc) {
		perror("malloc");
		exit(1);
	}
	proc->filename = filename;
	proc->pid = 0;
	proc->breakpoints = NULL;
	proc->breakpoints_enabled = -1;
	proc->callstack_depth = 0;
	proc->breakpoint_being_enabled = NULL;
	proc->next = NULL;
	if (opt_L && filename) {
		proc->list_of_symbols = read_elf(filename);
		if (opt_e) {
			struct library_symbol ** tmp1 = &(proc->list_of_symbols);
			while(*tmp1) {
				struct opt_e_t * tmp2 = opt_e;
				int keep = !opt_e_enable;

				while(tmp2) {
					if (!strcmp((*tmp1)->name, tmp2->name)) {
						keep = opt_e_enable;
					}
					tmp2 = tmp2->next;
				}
				if (!keep) {
					*tmp1 = (*tmp1)->next;
				} else {
					tmp1 = &((*tmp1)->next);
				}
			}
		}
	} else {
		proc->list_of_symbols = NULL;
	}
	sym = proc->list_of_symbols;
	while (sym) {
		insert_breakpoint(proc, sym->enter_addr); /* proc->pid==0 delays enabling. */
		sym = sym->next;
	}

	proc->next = list_of_processes;
	list_of_processes = proc;
	return proc;
}

void
open_pid(pid_t pid, int verbose) {
	struct process * proc;
	char * filename;

	if (trace_pid(pid)<0) {
		fprintf(stderr, "Cannot attach to pid %u: %s\n", pid, strerror(errno));
		return;
	}

	filename = pid2name(pid);

#if 0
	if (!filename) {
		if (verbose) {
			fprintf(stderr, "Cannot trace pid %u: %s\n", pid, strerror(errno));
		}
		return;
	}
#endif

	proc = open_program(filename);
	proc->pid = pid;
}
