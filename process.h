#ifndef _LTRACE_PROCESS_H
#define _LTRACE_PROCESS_H

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
	char * filename;	/* from execve() */
	int pid;
	int syscall_number;	/* outside syscall => syscall_number=-1 */
	struct process * next;
};

extern struct process * list_of_processes;

int execute_process(const char * file, char * const argv[]);
void init_sighandler(void);

#endif
