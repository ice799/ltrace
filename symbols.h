#ifndef _LTRACE_SYMBOLS_H
#define _LTRACE_SYMBOLS_H

#include "i386.h"

struct library_symbol {
	char * name;
	struct breakpoint sbp;
	unsigned long return_addr;
	struct library_symbol * next;
};

extern struct library_symbol * library_symbols;

void enable_all_breakpoints(int pid);
void disable_all_breakpoints(int pid);

#endif
