#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"
#include "sysdep.h"

static void change_uid(struct process * proc);

void execute_program(struct process * sp, char **argv)
{
	pid_t pid;

	if (opt_d) {
		output_line(0, "Executing `%s'...", sp->filename);
	}

	pid = fork();
	if (pid<0) {
		perror("fork");
		exit(1);
	} else if (!pid) {	/* child */
		change_uid(sp);
		trace_me();
		execvp(sp->filename, argv);
		fprintf(stderr, "Can't execute `%s': %s\n", sp->filename, strerror(errno));
		exit(1);
	}

	if (opt_d) {
		output_line(0, "PID=%d", pid);
	}

	sp->pid = pid;

	return;
}

static void change_uid(struct process * proc)
{
	uid_t run_uid, run_euid;
	gid_t run_gid, run_egid;

	if (opt_u) {
		struct passwd *pent;

		if (getuid() != 0 || geteuid() != 0) {
			fprintf(stderr, "you must be root to use the -u option\n");
			exit(1);
		}
		if ((pent = getpwnam(opt_u)) == NULL) {
			fprintf(stderr, "cannot find user `%s'\n", opt_u);
			exit(1);
		}
		run_uid = pent->pw_uid;
		run_gid = pent->pw_gid;

		if (initgroups(opt_u, run_gid) < 0) {
			perror("initgroups");
			exit(1);
		}
	} else {
		run_uid = getuid();
		run_gid = getgid();
	}
	if (opt_u || !geteuid()) {
		struct stat statbuf;
		run_euid = run_uid;
		run_egid = run_gid;

		if (!stat(proc->filename, &statbuf)) {
			if (statbuf.st_mode & S_ISUID) {
				run_euid = statbuf.st_uid;
			}
			if (statbuf.st_mode & S_ISGID) {
				run_egid = statbuf.st_gid;
			}
		}
		if (setregid(run_gid, run_egid) < 0) {
			perror("setregid");
			exit(1);
		}
		if (setreuid(run_uid, run_euid) < 0) {
			perror("setreuid");
			exit(1);
		}
	}
}
