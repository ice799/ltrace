#include <gelf.h>
#include "ltrace.h"
#include "elf.h"
#include "debug.h"
#include "ptrace.h"
#include "options.h"

GElf_Addr
arch_plt_sym_val (struct ltelf *lte, size_t ndx, GElf_Rela *rela)
{
  return rela->r_offset;
}

void * plt2addr(struct process *proc, void ** plt)
{
  long  addr;

  debug(3, 0);

  if (proc->e_machine == EM_PPC || plt == 0)
                return (void *)plt;

  if (proc->pid == 0)
                return (void *)0;

  // On a PowerPC-64 system, a plt is three 64-bit words: the first is the
  // 64-bit address of the routine.  Before the PLT has been initialized, this
  // will be 0x0. In fact, the symbol table won't have the plt's address even.
  // Ater the PLT has been initialized, but before it has been resolved, the
  // first word will be the address of the function in the dynamic linker that
  // will reslove the PLT.  After the PLT is resolved, this will will be the
  // address of the routine whose symbol is in the symbol table.

  addr = ptrace(PTRACE_PEEKTEXT, proc->pid, plt);

  if (opt_d >= 3) {
     xinfdump(proc->pid, plt, sizeof(void*)*3);
  }

  return (void *)addr;
}

