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

struct proc_arch {
	int syscall_number;		/* outside syscall => -1 */
};

#define PROC_BREAKPOINT 1
#define PROC_SYSCALL 2
#define PROC_SYSRET 3
int type_of_stop(int pid, struct proc_arch *proc_arch, int *what);
void proc_arch_init(struct proc_arch *proc_arch);

#endif
