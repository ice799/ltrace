#include <gelf.h>
#include "main.h"
#include "elf.h"

GElf_Addr
arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela * rela) {
	return lte->plt_addr + (ndx + 1)
	    * ((lte->ehdr.e_flags & EF_CPU32) ? 24 : 12);
}

void *
sym2addr(Process *proc, struct library_symbol *sym) {
	return sym->enter_addr;
}
