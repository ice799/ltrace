#ifndef _LTRACE_I386_H
#define _LTRACE_I386_H

#define BREAKPOINT {0xcc}
#define BREAKPOINT_LENGTH 1

void insert_breakpoint(int pid, unsigned long addr, unsigned char * value);
void delete_breakpoint(int pid, unsigned long addr, unsigned char * value);
unsigned long get_eip(int pid);
unsigned long get_esp(int pid);
unsigned long get_orig_eax(int pid);
unsigned long get_return(int pid, unsigned long esp);
unsigned long get_arg(int pid, unsigned long esp, int arg_num);
int is_there_a_breakpoint(int pid, unsigned long eip);
void continue_after_breakpoint(int pid, unsigned long eip, unsigned char * value, int delete_it);
void continue_process(int pid, int signal);
void trace_me(void);
void untrace_pid(int pid);

#include "process.h"
#define PROC_BREAKPOINT 1
#define PROC_SYSCALL 2
#define PROC_SYSRET 3
int type_of_stop(struct process * proc, int *what);

#endif
