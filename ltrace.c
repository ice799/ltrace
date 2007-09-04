#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include "ltrace.h"
#include "output.h"
#include "read_config_file.h"
#include "options.h"
#include "debug.h"

char *command = NULL;
struct process *list_of_processes = NULL;

int exiting = 0;		/* =1 if a SIGINT or SIGTERM has been received */

static void signal_alarm(int sig)
{
	struct process *tmp = list_of_processes;

	signal(SIGALRM, SIG_DFL);
	while (tmp) {
		struct opt_p_t *tmp2 = opt_p;
		while (tmp2) {
			if (tmp->pid == tmp2->pid) {
				tmp = tmp->next;
				if (!tmp) {
					return;
				}
				tmp2 = opt_p;
				continue;
			}
			tmp2 = tmp2->next;
		}
		debug(2, "Sending SIGSTOP to process %u\n", tmp->pid);
		kill(tmp->pid, SIGSTOP);
		tmp = tmp->next;
	}
}

static void signal_exit(int sig)
{
	exiting = 1;
	debug(1, "Received interrupt signal; exiting...");
	if (opt_o) {
		fclose(output);
		opt_o = 0;
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGALRM, signal_alarm);
	if (opt_p) {
		struct opt_p_t *tmp = opt_p;
		while (tmp) {
			debug(2, "Sending SIGSTOP to process %u\n", tmp->pid);
			kill(tmp->pid, SIGSTOP);
			tmp = tmp->next;
		}
	}
	alarm(1);
}

static void normal_exit(void)
{
	output_line(0, 0);
	if (opt_c) {
		show_summary();
	}
	if (opt_o) {
		fclose(output);
	}
}

static void guess_cols(void)
{
	struct winsize ws;
	char *c;

	opt_a = DEFAULT_ACOLUMN;
	c = getenv("COLUMNS");
	if (c && *c) {
		char *endptr;
		int cols;
		cols = strtol(c, &endptr, 0);
		if (cols > 0 && !*endptr) {
			opt_a = cols * 5 / 8;
		}
	} else if (ioctl(1, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0) {
		opt_a = ws.ws_col * 5 / 8;
	}
}

int main(int argc, char **argv)
{
	struct opt_p_t *opt_p_tmp;

	atexit(normal_exit);
	signal(SIGINT, signal_exit);	/* Detach processes when interrupted */
	signal(SIGTERM, signal_exit);	/*  ... or killed */

	guess_cols();
	argv = process_options(argc, argv);
	while (opt_F) {
		/* If filename begins with ~, expand it to the user's home */
		/* directory. This does not correctly handle ~yoda, but that */
		/* isn't as bad as it seems because the shell will normally */
		/* be doing the expansion for us; only the hardcoded */
		/* ~/.ltrace.conf should ever use this code. */
		if (opt_F->filename[0] == '~') {
			char path[PATH_MAX];
			char *home_dir = getenv("HOME");
			if (home_dir) {
				strncpy(path, home_dir, PATH_MAX - 1);
				path[PATH_MAX - 1] = '\0';
				strncat(path, opt_F->filename + 1,
						PATH_MAX - strlen(path) - 1);
				read_config_file(path);
			}
		} else {
			read_config_file(opt_F->filename);
		}
		opt_F = opt_F->next;
	}
	if (opt_e) {
		struct opt_e_t *tmp = opt_e;
		while (tmp) {
			debug(1, "Option -e: %s\n", tmp->name);
			tmp = tmp->next;
		}
	}
	if (command) {
		execute_program(open_program(command, 0), argv);
	}
	opt_p_tmp = opt_p;
	while (opt_p_tmp) {
		open_pid(opt_p_tmp->pid, 1);
		opt_p_tmp = opt_p_tmp->next;
	}
	while (1) {
		process_event(wait_for_something());
	}
}
