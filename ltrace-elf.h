#ifndef LTRACE_ELF_H
#define LTRACE_ELF_H

#include <gelf.h>
#include <stdlib.h>

struct ltelf {
	int fd;
	Elf *elf;
	GElf_Ehdr ehdr;
	Elf_Data *dynsym;
	size_t dynsym_count;
	const char *dynstr;
	GElf_Addr plt_addr;
	size_t plt_size;
	Elf_Data *relplt;
	size_t relplt_count;
	Elf_Data *symtab;
	const char *strtab;
	size_t symtab_count;
	Elf_Data *opd;
	GElf_Addr *opd_addr;
	size_t opd_size;
	Elf32_Word *hash;
	int hash_type;
	int lte_flags;
	GElf_Addr dyn_addr;
	size_t dyn_sz;
	GElf_Addr base_addr;
#ifdef __mips__
	size_t pltgot_addr;
	size_t mips_local_gotno;
	size_t mips_gotsym;
#endif // __mips__
	GElf_Addr plt_stub_vma;
};

#define ELF_MAX_SEGMENTS  50
#define LTE_HASH_MALLOCED 1
#define LTE_PLT_EXECUTABLE 2

#define PLTS_ARE_EXECUTABLE(lte) ((lte->lte_flags & LTE_PLT_EXECUTABLE) != 0)

extern size_t library_num;
extern char *library[MAX_LIBRARIES];

extern struct library_symbol *read_elf(Process *);

extern GElf_Addr arch_plt_sym_val(struct ltelf *, size_t, GElf_Rela *);

#ifndef SHT_GNU_HASH
#define SHT_GNU_HASH	0x6ffffff6	/* GNU-style hash table. */
#endif

#if __WORDSIZE == 32
#define PRI_ELF_ADDR		PRIx32
#define GELF_ADDR_CAST(x)	(void *)(uint32_t)(x)
#else
#define PRI_ELF_ADDR		PRIx64
#define GELF_ADDR_CAST(x)	(void *)(x)
#endif

#endif
