#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

/*
 * Returns a file name corresponding to a running pid
 */
char * pid2name(pid_t pid)
{
	char proc_exe[1024];

	if (!kill(pid, 0)) {
		sprintf(proc_exe, "/proc/%d/exe", pid);
		return strdup(proc_exe);
	} else {
		return NULL;
	}
}
