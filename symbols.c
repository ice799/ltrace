/*
 * This file contains functions related to the library_symbol struct
 */

#include <stdio.h>

#include "symbols.h"
#include "i386.h"

struct library_symbol * library_symbols = NULL;

void enable_all_breakpoints(int pid)
{
	struct library_symbol * tmp = NULL;

	tmp = library_symbols;
	while(tmp) {
		insert_breakpoint(pid, &tmp->sbp);
		tmp = tmp->next;
	}
}

void disable_all_breakpoints(int pid)
{
	struct library_symbol * tmp = NULL;

	tmp = library_symbols;
	while(tmp) {
		delete_breakpoint(pid, &tmp->sbp);
		tmp = tmp->next;
	}
}

