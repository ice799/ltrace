#define BREAKPOINT {0xcc}
#define BREAKPOINT_LENGTH 1

void insert_breakpoint(int pid, unsigned long addr, unsigned char * value);
void delete_breakpoint(int pid, unsigned long addr, unsigned char * value);
unsigned long get_eip(int pid);
int is_there_a_breakpoint(int pid, unsigned long eip);
void continue_after_breakpoint(int pid, unsigned long eip, unsigned char * value, int delete_it);
