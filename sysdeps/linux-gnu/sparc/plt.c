#include <gelf.h>
#include "ltrace.h"
#include "elf.h"

GElf_Addr
arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela * rela) {
	return rela->r_offset + 4;
}

void *
sym2addr(struct process *proc, struct library_symbol *sym) {
	return sym->enter_addr;
}
