#ifndef LTRACE_ELF_H
#define LTRACE_ELF_H

#include <elf.h>
#include "ltrace.h"

#if ELFSIZE == 64
#define Elf_Sym  Elf64_Sym
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#else
#define Elf_Sym  Elf32_Sym
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#endif

struct ltelf {
	int		fd;
	void*		maddr;
	Elf_Ehdr*	ehdr;
	char*		strtab;
	Elf_Sym*	symtab;
	int		symtab_len;
};

extern int library_num;
extern char *library[MAX_LIBRARY];
extern struct ltelf library_lte[MAX_LIBRARY];

extern struct library_symbol * read_elf(const char *);

#endif
