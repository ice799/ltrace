#ifndef LTRACE_ELF_H
#define LTRACE_ELF_H

#include <elf.h>
#include "ltrace.h"

struct ltelf {
	int		fd;
	void*		maddr;
	Elf32_Ehdr*	ehdr;
	char*		strtab;
	Elf32_Sym*	symtab;
	int		symtab_len;
};

extern int library_num;
extern char *library[MAX_LIBRARY];
extern struct ltelf library_lte[MAX_LIBRARY];

extern struct library_symbol * read_elf(const char *);

#endif
