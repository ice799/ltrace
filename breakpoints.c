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

struct breakpoint *address2bpstruct(struct process *proc, void *addr)
{
	return dict_find_entry(proc->breakpoints, addr);
}

void
insert_breakpoint(struct process *proc, void *addr,
		  struct library_symbol *libsym)
{
	struct breakpoint *sbp;
	debug(1, "symbol=%s, addr=%p", libsym?libsym->name:"(nil)", addr);

	if (!addr)
		return;

	if (libsym)
		libsym->needs_init = 0;

	sbp = dict_find_entry(proc->breakpoints, addr);
	if (!sbp) {
		sbp = calloc(1, sizeof(struct breakpoint));
		if (!sbp) {
			return;	/* TODO FIXME XXX: error_mem */
		}
		dict_enter(proc->breakpoints, addr, sbp);
		sbp->addr = addr;
		sbp->libsym = libsym;
		if (libsym)
			libsym->brkpnt = sbp;
	}
	sbp->enabled++;
	if (sbp->enabled == 1 && proc->pid)
		enable_breakpoint(proc->pid, sbp);
}

void delete_breakpoint(struct process *proc, void *addr)
{
	struct breakpoint *sbp = dict_find_entry(proc->breakpoints, addr);
	assert(sbp);		/* FIXME: remove after debugging has been done. */
	/* This should only happen on out-of-memory conditions. */
	if (sbp == NULL)
		return;

	sbp->enabled--;
	if (sbp->enabled == 0)
		disable_breakpoint(proc->pid, sbp);
	assert(sbp->enabled >= 0);
}

static void enable_bp_cb(void *addr, void *sbp, void *proc)
{
	if (((struct breakpoint *)sbp)->enabled) {
		enable_breakpoint(((struct process *)proc)->pid, sbp);
	}
}

void enable_all_breakpoints(struct process *proc)
{
	if (proc->breakpoints_enabled <= 0) {
#ifdef __powerpc__
		unsigned long a;

		/*
		 * PPC HACK! (XXX FIXME TODO)
		 * If the dynamic linker hasn't populated the PLT then
		 * dont enable the breakpoints
		 */
		if (opt_L) {
			a = ptrace(PTRACE_PEEKTEXT, proc->pid,
				   sym2addr(proc, proc->list_of_symbols),
				   0);
			if (a == 0x0)
				return;
		}
#endif

		debug(1, "Enabling breakpoints for pid %u...", proc->pid);
		if (proc->breakpoints) {
			dict_apply_to_all(proc->breakpoints, enable_bp_cb,
					  proc);
		}
#ifdef __mips__
		{
			// I'm sure there is a nicer way to do this. We need to
			// insert breakpoints _after_ the child has been started.
			struct library_symbol *sym;
			struct library_symbol *new_sym;
			sym=proc->list_of_symbols;
			while(sym){
				void *addr= sym2addr(proc,sym);
				if(!addr){
					sym=sym->next;
					continue;
				}
				if(dict_find_entry(proc->breakpoints,addr)){
					sym=sym->next;
					continue;
				}
				debug(2,"inserting bp %p %s",addr,sym->name);
				new_sym=malloc(sizeof(*new_sym));
				memcpy(new_sym,sym,sizeof(*new_sym));
				new_sym->next=proc->list_of_symbols;
				proc->list_of_symbols=new_sym;
				new_sym->brkpnt=0;
				insert_breakpoint(proc, addr, new_sym);
				sym=sym->next;
			}
		}
#endif
	}
	proc->breakpoints_enabled = 1;
}

static void disable_bp_cb(void *addr, void *sbp, void *proc)
{
	if (((struct breakpoint *)sbp)->enabled) {
		disable_breakpoint(((struct process *)proc)->pid, sbp);
	}
}

void disable_all_breakpoints(struct process *proc)
{
	if (proc->breakpoints_enabled) {
		debug(1, "Disabling breakpoints for pid %u...", proc->pid);
		dict_apply_to_all(proc->breakpoints, disable_bp_cb, proc);
	}
	proc->breakpoints_enabled = 0;
}

static void free_bp_cb(void *addr, void *sbp, void *data)
{
	assert(sbp);
	free(sbp);
}

void breakpoints_init(struct process *proc)
{
	struct library_symbol *sym;

	if (proc->breakpoints) {	/* let's remove that struct */
		dict_apply_to_all(proc->breakpoints, free_bp_cb, NULL);
		dict_clear(proc->breakpoints);
		proc->breakpoints = NULL;
	}
	proc->breakpoints = dict_init(dict_key2hash_int, dict_key_cmp_int);

	if (opt_L && proc->filename) {
		proc->list_of_symbols = read_elf(proc);
		if (opt_e) {
			struct library_symbol **tmp1 = &(proc->list_of_symbols);
			while (*tmp1) {
				struct opt_e_t *tmp2 = opt_e;
				int keep = !opt_e_enable;

				while (tmp2) {
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
	for (sym = proc->list_of_symbols; sym; sym = sym->next) {
		/* proc->pid==0 delays enabling. */
		insert_breakpoint(proc, sym2addr(proc, sym), sym);
	}
	proc->callstack_depth = 0;
	proc->breakpoints_enabled = -1;
}

void reinitialize_breakpoints(struct process *proc)
{
	struct library_symbol *sym = proc->list_of_symbols;

	while (sym) {
		if (sym->needs_init) {
			insert_breakpoint(proc, sym2addr(proc, sym),
					  sym);
			if (sym->needs_init && !sym->is_weak) {
				fprintf(stderr,
					"could not re-initialize breakpoint for \"%s\" in file \"%s\"\n",
					sym->name, proc->filename);
				exit(1);
			}
		}
		sym = sym->next;
	}
}
