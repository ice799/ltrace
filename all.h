/* elf.c: */
extern int read_elf(const char *);


/*
 *	Lista de types:
 */

#define	_T_INT		1
#define	_T_ADDR		6

#define	_T_UNKNOWN	-1
#define	_T_VOID		0
#define	_T_INT		1
#define	_T_UINT		2
#define	_T_OCTAL	3
#define	_T_CHAR		4
#define	_T_STRING	5
#define	_T_ADDR		6
#define	_T_FILE		7
#define	_T_HEX		8
#define	_T_FORMAT	9		/* printf-like format */

#define	_T_OUTPUT	0x80		/* OR'ed if arg is an OUTPUT value */

struct function {
	const char * function_name;
	int return_type;
	int num_params;
	int params_type[10];
	struct function * next;
};

extern struct function * list_of_functions;

extern void print_function(const char *, int pid, int esp);
#ifndef _LTRACE_I386_H
#define _LTRACE_I386_H

#define BREAKPOINT {0xcc}
#define BREAKPOINT_LENGTH 1

struct breakpoint {
	unsigned long addr;
	unsigned char value[BREAKPOINT_LENGTH];
};

void insert_breakpoint(int pid, struct breakpoint * sbp);
void delete_breakpoint(int pid, struct breakpoint * sbp);
unsigned long get_eip(int pid);
unsigned long get_esp(int pid);
unsigned long get_orig_eax(int pid);
unsigned long get_return(int pid, unsigned long esp);
unsigned long get_arg(int pid, unsigned long esp, int arg_num);
int is_there_a_breakpoint(int pid, unsigned long eip);
void continue_after_breakpoint(int pid, struct breakpoint * sbp, int delete_it);
void continue_process(int pid, int signal);
void trace_me(void);
void untrace_pid(int pid);

#include "process.h"
#define PROC_BREAKPOINT 1
#define PROC_SYSCALL 2
#define PROC_SYSRET 3
int type_of_stop(struct process * proc, int *what);

#endif
#include <stdio.h>

extern FILE * output;
extern int opt_d;
extern int opt_i;
extern int opt_S;
void send_left(const char * fmt, ...);
void send_right(const char * fmt, ...);
void send_line(const char * fmt, ...);
#ifndef _LTRACE_PROCESS_H
#define _LTRACE_PROCESS_H

#include "i386.h"

/* not ready yet */
#if 0
struct symbols_from_filename {
	char * filename;
	struct library_symbol * list_of_symbols;
}

struct library_symbol {
	char * name;
	unsigned long addr;
	unsigned long return_addr;
	unsigned char old_value[BREAKPOINT_LENGTH];
	struct library_symbol * next;
};
#endif

struct process {
	char * filename;		/* from execve() (TODO) */
	int pid;
	int breakpoints_enabled;
	struct breakpoint return_value;	/* if within a function */
	int syscall_number;		/* outside syscall => -1 */
	struct process * next;
};

extern struct process * list_of_processes;

unsigned int instruction_pointer;

int execute_process(const char * file, char * const argv[]);
void wait_for_child(void);

#endif
extern char * signal_name[];
#ifndef _LTRACE_SYMBOLS_H
#define _LTRACE_SYMBOLS_H

#include "i386.h"

struct library_symbol {
	char * name;
	struct breakpoint sbp;
	unsigned long return_addr;
	struct library_symbol * next;
};

extern struct library_symbol * library_symbols;

void enable_all_breakpoints(int pid);
void disable_all_breakpoints(int pid);

#endif

extern char * syscall_list[];
