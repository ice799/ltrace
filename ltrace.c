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

#include "ltrace.h"
#include "output.h"
#include "read_config_file.h"
#include "options.h"

char * command = NULL;
struct process * list_of_processes = NULL;

int exiting=0;				/* =1 if a SIGINT or SIGTERM has been received */

static void
signal_alarm(int sig) {
	struct process * tmp = list_of_processes;

	signal(SIGALRM,SIG_DFL);
	while(tmp) {
		struct opt_p_t * tmp2 = opt_p;
		while(tmp2) {
			if (tmp->pid == tmp2->pid) {
				tmp = tmp->next;
				if (!tmp) {
					return;
				}
				break;
			}
			tmp2 = tmp2->next;
		}
		if (opt_d>1) {
			output_line(0,"Sending SIGSTOP to process %u\n",tmp->pid);
		}
		kill(tmp->pid, SIGSTOP);
		tmp = tmp->next;
	}
}

static void
signal_exit(int sig) {
	exiting=1;
	if (opt_d) {
		output_line(0,"Received interrupt signal; exiting...");
	}
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGALRM,signal_alarm);
	if (opt_p) {
		struct opt_p_t * tmp = opt_p;
		while(tmp) {
			if (opt_d>1) {
				output_line(0,"Sending SIGSTOP to process %u\n",tmp->pid);
			}
			kill(tmp->pid, SIGSTOP);
			tmp = tmp->next;
		}
	}
	alarm(1);
}

static void
normal_exit(void) {
	output_line(0,0);
}

int
main(int argc, char **argv) {
	struct opt_p_t * opt_p_tmp;

	atexit(normal_exit);
	signal(SIGINT,signal_exit);	/* Detach processes when interrupted */
	signal(SIGTERM,signal_exit);	/*  ... or killed */

	argv = process_options(argc, argv);
	read_config_file("/etc/ltrace.conf");
	if (getenv("HOME")) {
		char path[PATH_MAX];
		sprintf(path, getenv("HOME"));	/* FIXME: buffer overrun */
		strcat(path, "/.ltrace.conf");
		read_config_file(path);
	}
	if (opt_d && opt_e) {
		struct opt_e_t * tmp = opt_e;
		while(tmp) {
			printf("Option -e: %s\n", tmp->name);
			tmp = tmp->next;
		}
	}
	if (command) {
		execute_program(open_program(command), argv);
	}
	opt_p_tmp = opt_p;
	while (opt_p_tmp) {
		open_pid(opt_p_tmp->pid, 1);
		opt_p_tmp = opt_p_tmp->next;
	}
	while(1) {
		process_event(wait_for_something());
	}
}
