#ifndef LTRACE_ELF_H
#define LTRACE_ELF_H

#include <gelf.h>
#include <stdlib.h>

#include "ltrace.h"

struct ltelf
{
  int fd;
  Elf *elf;
  GElf_Ehdr ehdr;
  Elf_Data *dynsym;
  size_t dynsym_count;
  const char *dynstr;
  GElf_Addr plt_addr;
  Elf_Data *relplt;
  size_t relplt_count;
  Elf32_Word *hash;
  int hash_malloced;
};

extern int library_num;
extern char *library[MAX_LIBRARY];

extern struct library_symbol *read_elf (const char *);

extern GElf_Addr arch_plt_sym_val (struct ltelf *, size_t, GElf_Rela *);

#endif
