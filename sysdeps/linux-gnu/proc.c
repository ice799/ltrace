#include "config.h"
#include "common.h"

#include <sys/types.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* /proc/pid doesn't exist just after the fork, and sometimes `ltrace'
 * couldn't open it to find the executable.  So it may be necessary to
 * have a bit delay
 */

#define	MAX_DELAY	100000	/* 100000 microseconds = 0.1 seconds */

/*
 * Returns a (malloc'd) file name corresponding to a running pid
 */
char *
pid2name(pid_t pid) {
	char proc_exe[1024];

	if (!kill(pid, 0)) {
		int delay = 0;

		sprintf(proc_exe, "/proc/%d/exe", pid);

		while (delay < MAX_DELAY) {
			if (!access(proc_exe, F_OK)) {
				return strdup(proc_exe);
			}
			delay += 1000;	/* 1 milisecond */
		}
	}
	return NULL;
}

static int
find_dynamic_entry_addr(Process *proc, void *pvAddr, int d_tag, void **addr) {
	int i = 0, done = 0;
	ElfW(Dyn) entry;

	debug(DEBUG_FUNCTION, "find_dynamic_entry()");

	if (addr ==	NULL || pvAddr == NULL || d_tag < 0 || d_tag > DT_NUM) {
		return -1;
	}

	while ((!done) && (i < ELF_MAX_SEGMENTS) &&
		(sizeof(entry) == umovebytes(proc, pvAddr, &entry, sizeof(entry))) &&
		(entry.d_tag != DT_NULL)) {

		if (entry.d_tag == d_tag) {
			done = 1;
			*addr = (void *)entry.d_un.d_val;
		}
		pvAddr += sizeof(entry);
		i++;
	}

	if (done) {
		debug(2, "found address: 0x%p in dtag %d\n", *addr, d_tag);
		return 0;
	}
	else {
		debug(2, "Couldn't address for dtag!\n");
		return -1;
	}
}

static void
crawl_linkmap(Process *proc, struct ltelf *lte, struct link_map *lm) {
	struct link_map rlm;
	char lib_name[BUFSIZ];

	debug (DEBUG_FUNCTION, "crawl_linkmap()");

	while (lm) {
		if (umovebytes(proc, lm, &rlm, sizeof(rlm)) != sizeof(rlm)) {
			debug(2, "Unable to read link map\n");
			return;
		}

		lm = rlm.l_next;
		if (rlm.l_name == NULL) {
			debug(2, "Invalid library name referenced in dynamic linker map\n");
			return;
		}

		umovebytes(proc, rlm.l_name, lib_name, sizeof(lib_name));

		if (lib_name[0] == '\0') {
			debug(2, "Library name is an empty string");
			continue;
		}

		debug(2, "Object %s, Loaded at 0x%x\n", lib_name, rlm.l_addr);

		if (library_num < MAX_LIBRARIES) {
			library[library_num++] = strdup(lib_name);
			lte[library_num].base_addr = rlm.l_addr;
		}
		else {
			fprintf (stderr, "MAX LIBS REACHED\n");
			exit(EXIT_FAILURE);
		}
	}
	return;
}

void
linkmap_init(Process *proc, struct ltelf *lte) {
	void *dbg_addr = NULL;
	struct r_debug rdbg, *tdbg = NULL;
	struct link_map *tlink_map = NULL;

	debug(DEBUG_FUNCTION, "linkmap_init()");

	if (find_dynamic_entry_addr(proc, (void *)lte->dyn_addr, DT_DEBUG, &dbg_addr) == -1) {
		debug(2, "Couldn't find debug structure!");
		return;
	}

	tdbg = dbg_addr;

	/* XXX
	 * use r_debug's s_brk to watch for libs as they are added/removed
	 */

	if (umovebytes(proc, tdbg, &rdbg, sizeof(rdbg)) != sizeof(rdbg)) {
		debug(2, "This process does not have a debug structure!\n");
		return;
	}

	tlink_map = rdbg.r_map;
	crawl_linkmap(proc, lte, tlink_map);

	return;
}
