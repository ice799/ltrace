#ifndef _LTRACE_SYMBOLS_H
#define _LTRACE_SYMBOLS_H

#include "i386.h"

struct library_symbol {
	char * name;
	unsigned long addr;
	unsigned long return_addr;
	unsigned char old_value[BREAKPOINT_LENGTH];
	struct library_symbol * next;
};

extern struct library_symbol * library_symbols;

void enable_all_breakpoints(int pid);
void disable_all_breakpoints(int pid);

#endif
