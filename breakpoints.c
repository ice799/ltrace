#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "ltrace.h"
#include "options.h"
#include "output.h"

#include <stdlib.h>
#include <assert.h>

#ifdef __powerpc__
#include <sys/ptrace.h>
#endif

/*****************************************************************************/

/*
  Dictionary code done by Morten Eriksen <mortene@sim.no>.

  FIXME: should we merge with dictionary code in demangle.c? 19990704 mortene.
*/

struct dict_entry {
	struct process * proc;
	struct breakpoint brk; /* addr field of struct is the hash key. */
	struct dict_entry * next;
};

#define DICTTABLESIZE 997 /* Semi-randomly selected prime number. */
static struct dict_entry * dict_buckets[DICTTABLESIZE];
static int dict_initialized = 0;

static void dict_init(void);
static void dict_clear(void);
static struct breakpoint * dict_enter(struct process * proc, void * brkaddr);
struct breakpoint * dict_find_entry(struct process * proc, void * brkaddr);
static void dict_apply_to_all(void (* func)(struct process *, struct breakpoint *, void * data), void * data);


static void
dict_init(void) {
	int i;
	/* FIXME: is this necessary? Check with ANSI C spec. 19990702 mortene. */
	for (i = 0; i < DICTTABLESIZE; i++) dict_buckets[i] = NULL;
	dict_initialized = 1;
}

static void
dict_clear(void) {
	int i;
	struct dict_entry * entry, * nextentry;

	for (i = 0; i < DICTTABLESIZE; i++) {
		for (entry = dict_buckets[i]; entry != NULL; entry = nextentry) {
			nextentry = entry->next;
			free(entry);
		}
		dict_buckets[i] = NULL;
	}
}

static struct breakpoint *
dict_enter(struct process * proc, void * brkaddr) {
	struct dict_entry * entry, * newentry;
	unsigned int bucketpos = ((unsigned long int)brkaddr) % DICTTABLESIZE;

	newentry = malloc(sizeof(struct dict_entry));
	if (!newentry) {
		perror("malloc");
		return NULL;
	}

	newentry->proc = proc;
	newentry->brk.addr = brkaddr;
	newentry->brk.enabled = 0;
	newentry->next = NULL;

	entry = dict_buckets[bucketpos];
	while (entry && entry->next) entry = entry->next;

	if (entry) entry->next = newentry;
	else dict_buckets[bucketpos] = newentry;

	if (opt_d > 2)
		output_line(0, "new brk dict entry at %p\n", brkaddr);

	return &(newentry->brk);
}

struct breakpoint *
dict_find_entry(struct process * proc, void * brkaddr) {
	unsigned int bucketpos = ((unsigned long int)brkaddr) % DICTTABLESIZE;
	struct dict_entry * entry = dict_buckets[bucketpos];
	while (entry) {
		if ((entry->brk.addr == brkaddr) && (entry->proc == proc)) break;
		entry = entry->next;
	}
	return entry ? &(entry->brk) : NULL;
}

static void
dict_apply_to_all(void (* func)(struct process *, struct breakpoint *, void * data), void * data) {
	int i;

	for (i = 0; i < DICTTABLESIZE; i++) {
		struct dict_entry * entry = dict_buckets[i];
		while (entry) {
			func(entry->proc, &(entry->brk), data);
			entry = entry->next;
		}
	}
}

#undef DICTTABLESIZE

/*****************************************************************************/

struct breakpoint *
address2bpstruct(struct process * proc, void * addr) {
	return dict_find_entry(proc, addr);
}

void
insert_breakpoint(struct process * proc, void * addr) {
	struct breakpoint * sbp;

	if (!dict_initialized) {
		dict_init();
		atexit(dict_clear);
	}

	sbp = dict_find_entry(proc, addr);
	if (!sbp) sbp = dict_enter(proc, addr);
	if (!sbp) return;

	sbp->enabled++;
	if (sbp->enabled==1 && proc->pid) enable_breakpoint(proc->pid, sbp);
}

void
delete_breakpoint(struct process * proc, void * addr) {
	struct breakpoint * sbp = dict_find_entry(proc, addr);
	assert(sbp); /* FIXME: remove after debugging has been done. */
	/* This should only happen on out-of-memory conditions. */
	if (sbp == NULL) return;

	sbp->enabled--;
	if (sbp->enabled == 0) disable_breakpoint(proc->pid, sbp);
	assert(sbp->enabled >= 0);
}

static void
enable_bp_cb(struct process * proc, struct breakpoint * sbp, void * data) {
	struct process * myproc = (struct process *)data;
	if (myproc == proc && sbp->enabled) enable_breakpoint(proc->pid, sbp);
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
		a = ptrace(PTRACE_PEEKTEXT, proc->pid, proc->list_of_symbols->enter_addr, 0);
		if (a == 0x0)
			return;
#endif

		if (opt_d>0) {
			output_line(0, "Enabling breakpoints for pid %u...", proc->pid);
		}
		dict_apply_to_all(enable_bp_cb, proc);
	}
	proc->breakpoints_enabled = 1;
}

static void
disable_bp_cb(struct process * proc, struct breakpoint * sbp, void * data) {
	struct process * myproc = (struct process *)data;
	if (myproc == proc && sbp->enabled) disable_breakpoint(proc->pid, sbp);
}

void
disable_all_breakpoints(struct process * proc) {
	if (proc->breakpoints_enabled) {
		if (opt_d>0) {
			output_line(0, "Disabling breakpoints for pid %u...", proc->pid);
		}
		dict_apply_to_all(disable_bp_cb, proc);
	}
	proc->breakpoints_enabled = 0;
}
