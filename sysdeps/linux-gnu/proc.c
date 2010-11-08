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
