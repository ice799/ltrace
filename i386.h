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
void trace_me(void);
void continue_process(int pid, int signal);
