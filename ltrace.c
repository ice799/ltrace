#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "elf.h"
#include "process.h"
#include "output.h"

extern void read_config_file(const char *);

FILE * output = stderr;
int opt_d = 0;		/* debug */
int opt_i = 0;		/* instruction pointer */
int opt_S = 0;		/* syscalls */

static void usage(void)
{
	fprintf(stderr,"Usage: ltrace [-d] [-i] [-S] [-o filename] command [arg ...]\n\n");
}

static char * search_for_command(char * filename)
{
	static char pathname[MAXPATHLEN];
	char *path;
	int m, n;

	if (strchr(filename, '/')) {
		return filename;
	}
	for (path = getenv("PATH"); path && *path; path += m) {
		if (strchr(path, ':')) {
			n = strchr(path, ':') - path;
			m = n + 1;
		} else {
			m = n = strlen(path);
		}
		strncpy(pathname, path, n);
		if (n && pathname[n - 1] != '/') {
			pathname[n++] = '/';
		}
		strcpy(pathname + n, filename);
		if (!access(pathname, X_OK)) {
			break;
		}
	}
	if (access(pathname, X_OK)) {
		return NULL;
	} else {
		return pathname;
	}
}

int main(int argc, char **argv)
{
	int pid;
	char * command;

	while ((argc>2) && (argv[1][0] == '-') && (argv[1][2] == '\0')) {
		switch(argv[1][1]) {
			case 'd':	opt_d++;
					break;
			case 'o':	output = fopen(argv[2], "w");
					if (!output) {
						fprintf(stderr, "Can't open %s for output: %s\n", argv[2], sys_errlist[errno]);
						exit(1);
					}
					argc--; argv++;
					break;
			case 'i':	opt_i++;
					break;
			case 'S':	opt_S++;
					break;
			default:	fprintf(stderr, "Unknown option '%c'\n", argv[1][1]);
					usage();
					exit(1);
		}
		argc--; argv++;
	}

	if (argc<2) {
		usage();
		exit(1);
	}
	command = search_for_command(argv[1]);
	if (!command) {
		fprintf(stderr, "%s: command not found\n", argv[1]);
		exit(1);
	}
	if (!read_elf(command)) {
		fprintf(stderr, "%s: Not dynamically linked\n", command);
		exit(1);
	}

	if (opt_d>0) {
		send_line("Reading config file(s)...");
	}
	read_config_file("/etc/ltrace.cfg");
	read_config_file(".ltracerc");

	pid = execute_process(command, argv+1);
	if (opt_d>0) {
		send_line("pid %u launched", pid);
	}

	while(1) {
		wait_for_child();
	}

	exit(0);
}
