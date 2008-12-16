#include <gelf.h>
#include "ltrace.h"
#include "elf.h"
#include "debug.h"
#include "ptrace.h"
#include "options.h"

GElf_Addr
arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela * rela) {
	return rela->r_offset;
}

void *
sym2addr(struct process *proc, struct library_symbol *sym) {
	void *addr = sym->enter_addr;
	long pt_ret;

	debug(3, 0);

	if (sym->plt_type != LS_TOPLT_POINT) {
		return addr;
	}

	if (proc->pid == 0) {
		return 0;
	}

	if (opt_d >= 3) {
		xinfdump(proc->pid, (void *)(((long)addr-32)&0xfffffff0),
			 sizeof(void*)*8);
	}

	// On a PowerPC-64 system, a plt is three 64-bit words: the first is the
	// 64-bit address of the routine.  Before the PLT has been initialized,
	// this will be 0x0. In fact, the symbol table won't have the plt's
	// address even.  Ater the PLT has been initialized, but before it has
	// been resolved, the first word will be the address of the function in
	// the dynamic linker that will reslove the PLT.  After the PLT is
	// resolved, this will will be the address of the routine whose symbol
	// is in the symbol table.

	// On a PowerPC-32 system, there are two types of PLTs: secure (new) and
	// non-secure (old).  For the secure case, the PLT is simply a pointer
	// and we can treat it much as we do for the PowerPC-64 case.  For the
	// non-secure case, the PLT is executable code and we can put the
	// break-point right in the PLT.

	pt_ret = ptrace(PTRACE_PEEKTEXT, proc->pid, addr, 0);

	if (proc->mask_32bit) {
		// Assume big-endian.
		addr = (void *)((pt_ret >> 32) & 0xffffffff);
	} else {
		addr = (void *)pt_ret;
	}

	return addr;
}
