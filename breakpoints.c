#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __powerpc__
#include <sys/ptrace.h>
#endif

#include "ltrace.h"
#include "options.h"
#include "debug.h"
#include "dict.h"
#include "elf.h"

/*****************************************************************************/

struct breakpoint *
address2bpstruct(struct process * proc, void * addr) {
	return dict_find_entry(proc->breakpoints, addr);
}

void
insert_breakpoint(struct process * proc, void * addr) {
	struct breakpoint * sbp;

	if (!proc->breakpoints) {
		proc->breakpoints = dict_init(dict_key2hash_int, dict_key_cmp_int);
		/* atexit(brk_dict_clear); */ /* why bother to do this on exit? */
	}
	sbp = dict_find_entry(proc->breakpoints, addr);
	if (!sbp) {
		sbp = malloc(sizeof(struct breakpoint));
		if (!sbp) {
			return; /* TODO FIXME XXX: error_mem */
		}
		dict_enter(proc->breakpoints, addr, sbp);
		sbp->addr = addr;
		sbp->enabled = 0;
	}
	sbp->enabled++;
	if (sbp->enabled==1 && proc->pid) enable_breakpoint(proc->pid, sbp);
}

void
delete_breakpoint(struct process * proc, void * addr) {
	struct breakpoint * sbp = dict_find_entry(proc->breakpoints, addr);
	assert(sbp); /* FIXME: remove after debugging has been done. */
	/* This should only happen on out-of-memory conditions. */
	if (sbp == NULL) return;

	sbp->enabled--;
	if (sbp->enabled == 0) disable_breakpoint(proc->pid, sbp);
	assert(sbp->enabled >= 0);
}

static void
enable_bp_cb(void * addr, void * sbp, void * proc) {
	if (((struct breakpoint *)sbp)->enabled) {
		enable_breakpoint(((struct process *)proc)->pid, sbp);
	}
}

void
enable_all_breakpoints(struct process * proc) {
	if (proc->breakpoints_enabled <= 0) {
#ifdef __powerpc__
		unsigned long a;

		/*
		 * PPC HACK! (XXX FIXME TODO)
		 * If the dynamic linker hasn't populated the PLT then
		 * dont enable the breakpoints
		 */
		if (opt_L) {
			a = ptrace(PTRACE_PEEKTEXT, proc->pid, proc->list_of_symbols->enter_addr, 0);
			if (a == 0x0)
				return;
		}
#endif

		debug(1, "Enabling breakpoints for pid %u...", proc->pid);
		dict_apply_to_all(proc->breakpoints, enable_bp_cb, proc);
	}
	proc->breakpoints_enabled = 1;
}

static void
disable_bp_cb(void * addr, void * sbp, void * proc) {
	if (((struct breakpoint *)sbp)->enabled) {
		disable_breakpoint(((struct process *)proc)->pid, sbp);
	}
}

void
disable_all_breakpoints(struct process * proc) {
	if (proc->breakpoints_enabled) {
		debug(1, "Disabling breakpoints for pid %u...", proc->pid);
		dict_apply_to_all(proc->breakpoints, disable_bp_cb, proc);
	}
	proc->breakpoints_enabled = 0;
}

static void
free_bp_cb(void * addr, void * sbp, void * data) {
	assert(sbp);
	free(sbp);
}

void
breakpoints_init(struct process * proc) {
	struct library_symbol * sym;

	if (proc->breakpoints) { /* let's remove that struct */
		/* TODO FIXME XXX: free() all "struct breakpoint"s */
		dict_apply_to_all(proc->breakpoints, free_bp_cb, NULL);
		dict_clear(proc->breakpoints);
		proc->breakpoints = NULL;
	}

	if (opt_L && proc->filename) {
		proc->list_of_symbols = read_elf(proc->filename);
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
	proc->callstack_depth = 0;
	proc->breakpoints_enabled = -1;
}
