#ifndef _HCK_LTRACE_H
#define _HCK_LTRACE_H

#include <sys/types.h>
#include <stdio.h>

#include "defs.h"

/* BREAKPOINT_LENGTH is defined in "sysdep.h" */
#include "sysdep.h"

extern char * command;

struct breakpoint {
	void * addr;
	unsigned char orig_value[BREAKPOINT_LENGTH];
	int enabled;
};

enum param_type {
	LT_PT_UNKNOWN=-1,
	LT_PT_VOID,
	LT_PT_INT,
	LT_PT_UINT,
	LT_PT_OCTAL,
	LT_PT_CHAR,
	LT_PT_ADDR,
	LT_PT_FILE,
	LT_PT_FORMAT,		/* printf-like format */
	LT_PT_STRING,
	LT_PT_STRING0,		/* stringN: string up to (arg N) bytes */
	LT_PT_STRING1,
	LT_PT_STRING2,
	LT_PT_STRING3
};

enum tof {
	LT_TOF_NONE,
	LT_TOF_FUNCTION,	/* A real library function */
	LT_TOF_SYSCALL		/* A syscall */
};

struct function {
	const char * name;
	enum param_type return_type;
	int num_params;
	enum param_type param_types[MAX_ARGS];
	int params_right;
	struct function * next;
};

extern struct function * list_of_functions;

struct library_symbol {
	char * name;
	struct breakpoint brk;

	struct library_symbol * next;
};

struct process {
	char * filename;
	pid_t pid;
	int breakpoints_enabled;	/* -1:not enabled yet, 0:disabled, 1:enabled */

	int current_syscall;			/* -1 for none */
	struct library_symbol * current_symbol;	/* NULL for none */

	struct breakpoint return_value;
	struct library_symbol * list_of_symbols;

	/* Arch-dependent: */
	void * instruction_pointer;
	void * stack_pointer;		/* To get return addr, args... */
	void * return_addr;
	struct breakpoint * breakpoint_being_enabled;

	/* output: */
	enum tof type_being_displayed;

	struct process * next;
};

struct event {
	struct process *proc;
	enum {
		LT_EV_UNKNOWN,
		LT_EV_NONE,
		LT_EV_SIGNAL,
		LT_EV_EXIT,
		LT_EV_EXIT_SIGNAL,
		LT_EV_SYSCALL,
		LT_EV_SYSRET,
		LT_EV_BREAKPOINT
	} thing;
	union {
		int ret_val;		/* _EV_EXIT */
		int signum;		/* _EV_SIGNAL, _EV_EXIT_SIGNAL */
		int sysnum;		/* _EV_SYSCALL, _EV_SYSRET */
		void * brk_addr;	/* _EV_BREAKPOINT */
	} e_un;
};

extern struct process * list_of_processes;

extern void * instruction_pointer;

struct event * wait_for_something(void);
void process_event(struct event * event);
void execute_program(struct process *, char **);
int display_arg(enum tof type, struct process * proc, int arg_num, enum param_type rt);
void enable_all_breakpoints(struct process * proc);
void disable_all_breakpoints(struct process * proc);

/* Arch-dependent stuff: */
extern char * pid2name(pid_t pid);
extern void trace_me(void);
extern void trace_pid(pid_t pid);
extern void * get_instruction_pointer(int pid);
extern void * get_stack_pointer(int pid);
extern void * get_return_addr(int pid, void * stack_pointer);
extern void insert_breakpoint(int pid, struct breakpoint * sbp);
extern void delete_breakpoint(int pid, struct breakpoint * sbp);
extern int fork_p(int sysnum);
extern int exec_p(int sysnum);
extern int syscall_p(struct process * proc, int status);
extern int sysret_p(struct process * proc, int status);
extern void continue_process(pid_t pid);
extern void continue_after_signal(pid_t pid, int signum);
extern void continue_after_breakpoint(struct process * proc, struct breakpoint * sbp, int delete_it);
extern void continue_enabling_breakpoint(pid_t pid, struct breakpoint * sbp);
extern long gimme_arg(enum tof type, struct process * proc, int arg_num);
extern int umovestr(struct process * proc, void * addr, int len, void * laddr);
#if 0	/* not yet */
extern int umoven(struct process * proc, void * addr, int len, void * laddr);
#endif


#endif
