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

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif

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
		debug(2,"Sending SIGSTOP to process %u\n",tmp->pid);
		kill(tmp->pid, SIGSTOP);
		tmp = tmp->next;
	}
}

static void
signal_exit(int sig) {
	exiting=1;
	debug(1,"Received interrupt signal; exiting...");
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGALRM,signal_alarm);
	if (opt_p) {
		struct opt_p_t * tmp = opt_p;
		while(tmp) {
			debug(2,"Sending SIGSTOP to process %u\n",tmp->pid);
			kill(tmp->pid, SIGSTOP);
			tmp = tmp->next;
		}
	}
	alarm(1);
}

struct opt_c_struct2 {
	int num;
	struct {
		char *name;
		int count;
		struct timeval tv;
	} st[0];
};

static void
opt_c_show_func(void * key, void * value, void * data) {
	struct opt_c_struct * st = (struct opt_c_struct *)value;
//	unsigned long int c;
	struct opt_c_struct2 * st2 = (struct opt_c_struct2 *)data;
	int num = st2->num;

	st2 = realloc(st2, sizeof(struct opt_c_struct2)+ (num+1)*sizeof(st2->st[0]));
	if (!st2) {
		perror("realloc()");
		exit(1);
	}
	st2->num++;
	st2->st[num].name = (char *)key;
	st2->st[num].count = st->count;
	st2->st[num].tv = st->tv;
//	c = 1000000 * (int)st->tv.tv_sec + (int)st->tv.tv_usec;
//	c /= st->count;
//	printf("%3d.%02d %4d.%06d %11lu %9d %s\n", 99, 99,
//			(int)st->tv.tv_sec, (int)st->tv.tv_usec,
//			c, st->count, (char *)key);
}

static void
normal_exit(void) {
	output_line(0,0);
	if (opt_c) {
		struct opt_c_struct2 * st2 = malloc(sizeof(struct opt_c_struct2));

		if (!st2) {
			perror("malloc()");
			exit(1);
		}
		st2->num = 0;
		printf("%% time     seconds  usecs/call     calls      function\n");
		printf( "------ ----------- ----------- --------- --------------------\n");
		dict_apply_to_all(dict_opt_c, opt_c_show_func, st2);
		printf( "------ ----------- ----------- --------- --------------------\n");
		printf( "100.00    0.000360                    27 total\n");
	}
}

static void
guess_cols(void) {
	struct winsize ws;
	char * c;

	opt_a = DEFAULT_ACOLUMN;
	c = getenv("COLUMNS");
	if (c && *c) {
		char * endptr;
		int cols;
		cols = strtol(c, &endptr, 0);
		if (cols>0 && !*endptr) {
			opt_a = cols * 5/8;
		}
	} else if (ioctl(1, TIOCGWINSZ, &ws) != -1 && ws.ws_col>0) {
		opt_a = ws.ws_col * 5/8;
	}
}

int
main(int argc, char **argv) {
	struct opt_p_t * opt_p_tmp;
	char * home;

	atexit(normal_exit);
	signal(SIGINT,signal_exit);	/* Detach processes when interrupted */
	signal(SIGTERM,signal_exit);	/*  ... or killed */

	guess_cols();
	argv = process_options(argc, argv);
	read_config_file(SYSCONFDIR "/ltrace.conf");
	home = getenv("HOME");
	if (home) {
		char path[PATH_MAX];
		if (strlen(home) > PATH_MAX-15) {
			fprintf(stderr, "Error: $HOME too long\n");
			exit(1);
		}
		strcpy(path, getenv("HOME"));
		strcat(path, "/.ltrace.conf");
		read_config_file(path);
	}
	if (opt_e) {
		struct opt_e_t * tmp = opt_e;
		while(tmp) {
			debug(1,"Option -e: %s\n", tmp->name);
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
