#include "ltrace.h"
#include "options.h"
#include "output.h"

void enable_all_breakpoints(struct process * proc)
{
	if (proc->breakpoints_enabled <= 0) {
		struct library_symbol * tmp = proc->list_of_symbols;

		if (opt_d>0) {
			output_line(0, "Enabling breakpoints for pid %u...", proc->pid);
		}
		while(tmp) {
			insert_breakpoint(proc->pid, &tmp->brk);
			tmp = tmp->next;
		}
	}
	proc->breakpoints_enabled = 1;
}

void disable_all_breakpoints(struct process * proc)
{
	if (proc->breakpoints_enabled) {
		struct library_symbol * tmp = proc->list_of_symbols;

		if (opt_d>0) {
			output_line(0, "Disabling breakpoints for pid %u...", proc->pid);
		}
		while(tmp) {
			delete_breakpoint(proc->pid, &tmp->brk);
			tmp = tmp->next;
		}
	}
	proc->breakpoints_enabled = 0;
}
