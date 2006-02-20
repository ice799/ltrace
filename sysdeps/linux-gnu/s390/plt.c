#include <gelf.h>
#include "ltrace.h"
#include "elf.h"

GElf_Addr
arch_plt_sym_val (struct ltelf *lte, size_t ndx, GElf_Rela *rela)
{
  return lte->plt_addr + (ndx + 1) * 32;
}


void * plt2addr(struct process *proc, void ** plt)
{
  return (void *) plt;
}
