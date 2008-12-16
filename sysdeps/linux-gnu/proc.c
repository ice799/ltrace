#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
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
 * Returns a file name corresponding to a running pid
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
