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
	int within_function;
	struct breakpoint return_value;	/* if within a function */
	struct proc_arch proc_arch;
	struct process * next;
};

extern struct process * list_of_processes;

extern unsigned int instruction_pointer;

int execute_process(const char * file, char * const argv[]);
void wait_for_child(void);

#endif
