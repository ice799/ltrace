#include "i386.h"

struct library_symbol {
	char * name;
	unsigned long addr;
	unsigned char old_value[BREAKPOINT_LENGTH];
	struct library_symbol * next;
};

extern struct library_symbol * library_symbols;

void enable_all_breakpoints(int pid);
