#define BREAKPOINT {0xcc}
#define BREAKPOINT_LENGTH 1

void insert_breakpoint(int pid, unsigned long addr, unsigned char * value);
void delete_breakpoint(int pid, unsigned long addr, unsigned char * value);
