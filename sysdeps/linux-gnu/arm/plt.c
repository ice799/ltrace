#include <gelf.h>
#include "common.h"

static int
arch_plt_entry_has_stub(struct ltelf *lte, size_t off) {
	uint16_t op = *(uint16_t *)((char *)lte->relplt->d_buf + off);
	return op == 0x4778;
}

GElf_Addr
arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela * rela) {
	size_t start = lte->relplt->d_size + 12;
	size_t off = start + 20, i;
	for (i = 0; i < ndx; i++)
		off += arch_plt_entry_has_stub(lte, off) ? 16 : 12;
	if (arch_plt_entry_has_stub(lte, off))
		off += 4;
	return lte->plt_addr + off - start;
}

void *
sym2addr(Process *proc, struct library_symbol *sym) {
	return sym->enter_addr;
}
