#include <debug.h>
#include <gelf.h>
#include <sys/ptrace.h>
#include "common.h"
#include "ltrace_elf.h"

/**
   \addtogroup mipsel
   @{
 */

/**
   \param lte Structure containing link table entry information
   \param ndx Index into .dynsym
   \param rela Not used.
   \return Address of GOT table entry

   MIPS ABI Supplement:

   DT_PLTGOT This member holds the address of the .got section.

   DT_MIPS_SYMTABNO This member holds the number of entries in the
   .dynsym section.

   DT_MIPS_LOCAL_GOTNO This member holds the number of local global
   offset table entries.

   DT_MIPS_GOTSYM This member holds the index of the first dyamic
   symbol table entry that corresponds to an entry in the gobal offset
   table.

   Called by read_elf when building the symbol table.

 */
GElf_Addr
arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela * rela) {
    debug(1,"plt_addr %x ndx %#x",lte->pltgot_addr, ndx);
    return lte->pltgot_addr +
		sizeof(void *) * (lte->mips_local_gotno + (ndx - lte->mips_gotsym));
}
/**
   \param proc The process to work on.
   \param sym The library symbol.
   \return What is at the got table address

   The return value should be the address to put the breakpoint at.

   On the mips the library_symbol.enter_addr is the .got addr for the
   symbol and the breakpoint.addr is the actual breakpoint address.

   Other processors use a plt, the mips is "special" in that is uses
   the .got for both function and data relocations. Prior to program
   startup, return 0.

   \warning MIPS relocations are lazy. This means that the breakpoint
   may move after the first call. Ltrace dictionary routines don't
   have a delete and symbol is one to one with breakpoint, so if the
   breakpoint changes I just add a new breakpoint for the new address.
 */
void *
sym2addr(Process *proc, struct library_symbol *sym) {
    long ret;
    if(!proc->pid){
        return 0;
    }
    ret=ptrace(PTRACE_PEEKTEXT, proc->pid, sym->enter_addr, 0);
    if(ret==-1){
        ret =0;
    }
    return (void *)ret;;
}

/**@}*/
