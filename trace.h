#include "i386.h"

extern int pid;

struct library_symbol {
	char * name;
	unsigned long addr;
	unsigned char old_value[BREAKPOINT_LENGTH];
	struct library_symbol * next;
};

extern struct library_symbol * library_symbols;

int attach_process(const char * file, char * const argv[]);
void enable_all_breakpoints(void);
