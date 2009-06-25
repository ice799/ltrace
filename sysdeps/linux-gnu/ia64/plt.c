#include <gelf.h>
#include "common.h"
#include "elf.h"
#include "debug.h"

/* A bundle is 128 bits */
#define BUNDLE_SIZE 16

/*

  The PLT has

  ] 3 bundles as a header

  ] The special reserved entry

  ] Following that, each PLT entry has it's initial code that the GOT entry
    points to.  Each PLT entry has one bundle allocated.

  ] Following that, each PLT entry has two bundles of actual PLT code,
    i.e. load up the address from the GOT and jump to it.  This is the
    point we want to insert the breakpoint, as this will be captured
    every time we jump to the PLT entry in the code.

*/

GElf_Addr
arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela * rela) {
	/* Find number of entires by removing header and special
	 * entry, dividing total size by three, since each PLT entry
	 * will have 3 bundles (1 for inital entry and two for the PLT
	 * code). */
	int entries = (lte->plt_size - 4 * BUNDLE_SIZE) / (3 * BUNDLE_SIZE);

	/* Now the point we want to break on is the PLT entry after
	 * all the header stuff */
	unsigned long addr =
	    lte->plt_addr + (4 * BUNDLE_SIZE) + (BUNDLE_SIZE * entries) +
	    (2 * ndx * BUNDLE_SIZE);
	debug(3, "Found PLT %d entry at %lx\n", ndx, addr);

	return addr;
}

void *
sym2addr(Process *proc, struct library_symbol *sym) {
	return sym->enter_addr;
}
