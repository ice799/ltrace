#if HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * This file contains functions specific to ELF binaries
 *
 * Silvio Cesare <silvio@big.net.au>
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "ltrace.h"
#include "elf.h"
#include "debug.h"

static void do_init_elf(struct ltelf *lte, const char *filename);
static void do_close_elf(struct ltelf *lte);
static void do_load_elf_symtab(struct ltelf *lte);
static void do_init_load_libraries(void);
static void do_close_load_libraries(void);
static int in_load_libraries(const char *func);
static void add_library_symbol(
	struct ltelf *lte,
	int i,
	struct library_symbol **library_symbolspp
);

static struct ltelf library_lte[MAX_LIBRARY];

static void
do_init_elf(struct ltelf *lte, const char *filename) {
	struct stat sbuf;

	debug(1, "Reading ELF from %s...", filename);

	lte->fd = open(filename, O_RDONLY);
	if (lte->fd == -1) {
		fprintf(
			stderr,
			"Can't open \"%s\": %s\n",
			filename,
			strerror(errno)
		);
		exit(1);
	}
	if (fstat(lte->fd, &sbuf) == -1) {
		fprintf(
			stderr,
			"Can't stat \"%s\": %s\n",
			filename,
			strerror(errno)
		);
		exit(1);
	}
	if (sbuf.st_size < sizeof(Elf32_Ehdr)) {
		fprintf(
			stderr,
			"\"%s\" is not an ELF binary object\n",
			filename
		);
		exit(1);
	}
	lte->maddr = mmap(
		NULL, sbuf.st_size, PROT_READ, MAP_SHARED, lte->fd, 0
	);
	if (lte->maddr == (void*)-1) {
		fprintf(
			stderr,
			"Can't mmap \"%s\": %s\n",
			filename,
			strerror(errno)
		);
		exit(1);
	}

	lte->ehdr = lte->maddr;

	if (strncmp(lte->ehdr->e_ident, ELFMAG, SELFMAG)) {
		fprintf(
			stderr,
			"\"%s\" is not an ELF binary object\n",
			filename
		);
		exit(1);
	}

/*
	more ELF checks should go here - the e_arch/e_machine fields in the
	ELF header are specific to each architecture. perhaps move some code
	into sysdeps (have check_ehdr_arch) - silvio
*/

	lte->strtab = NULL;

	lte->symtab = NULL;
	lte->symtab_len = 0;
}

static void
do_close_elf(struct ltelf *lte) {
	close(lte->fd);
}

static void
do_load_elf_symtab(struct ltelf *lte) {
	void *maddr = lte->maddr;
	Elf32_Ehdr *ehdr = lte->ehdr;
	Elf32_Shdr *shdr = (Elf32_Shdr *)(maddr + ehdr->e_shoff);
	int i;

/*
	an ELF object should only ever one dynamic symbol section (DYNSYM), but
	can have multiple string tables.  the sh_link entry from DYNSYM points
	to the correct STRTAB section - silvio
*/

	for(i = 0; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type == SHT_DYNSYM) {
			lte->symtab = (Elf32_Sym *)(maddr + shdr[i].sh_offset);
			lte->symtab_len = shdr[i].sh_size;
			lte->strtab = (char *)(
				maddr + shdr[shdr[i].sh_link].sh_offset
			);
		}
	}

	debug(2, "symtab: 0x%08x", (unsigned)lte->symtab);
	debug(2, "symtab_len: %lu", lte->symtab_len);
	debug(2, "strtab: 0x%08x", (unsigned)lte->strtab);
}

static void
add_library_symbol(
		struct ltelf *lte,
		int i,
		struct library_symbol **library_symbolspp) {
	struct library_symbol *tmp = *library_symbolspp;
	struct library_symbol *library_symbols;

	*library_symbolspp = (struct library_symbol *)malloc(
		sizeof(struct library_symbol)
	);
	library_symbols = *library_symbolspp;
	if (library_symbols == NULL) {
		perror("ltrace: malloc");
		exit(1);
	}

	library_symbols->enter_addr = (void *)lte->symtab[i].st_value;
	library_symbols->name = &lte->strtab[lte->symtab[i].st_name];
	library_symbols->next = tmp;

	debug(2, "addr: 0x%08x, symbol: \"%s\"",
			(unsigned)lte->symtab[i].st_value,
			&lte->strtab[lte->symtab[i].st_name]);
}

/*
	this is all pretty slow. perhaps using .hash would be faster, or
	even just a custum built hash table. its all initialization though,
	so its not that bad - silvio
*/

static void
do_init_load_libraries(void) {
	int i;

	for (i = 0; i < library_num; i++) {
		do_init_elf(&library_lte[i], library[i]);
		do_load_elf_symtab(&library_lte[i]);
	}
}

static void
do_close_load_libraries(void) {
	int i;

	for (i = 0; i < library_num; i++) {
		do_close_elf(&library_lte[i]);
	}
}

static int
in_load_libraries(const char *func) {
	int i, j;
/*
	if no libraries are specified, assume we want all
*/
	if (library_num == 0) return 1;

	for (i = 0; i < library_num; i++) {
		Elf32_Sym *symtab = library_lte[i].symtab;
		char *strtab = library_lte[i].strtab;

		for(
			j = 0;
			j < library_lte[i].symtab_len / sizeof(Elf32_Sym);
			j++
		) {
			if (
				symtab[j].st_value &&
				!strcmp(func, &strtab[symtab[j].st_name])
			) return 1;
		}
	}
	return 0;
}

/*
	this is the main function
*/

struct library_symbol *
read_elf(const char *filename) {
	struct library_symbol *library_symbols = NULL;
	struct ltelf lte;
	int i;

	do_init_elf(&lte, filename);
	do_load_elf_symtab(&lte);
	do_init_load_libraries();

	for(i = 0; i < lte.symtab_len / sizeof(Elf32_Sym); i++) {
		Elf32_Sym *symtab = lte.symtab;
		char *strtab = lte.strtab;

		if (!symtab[i].st_shndx && symtab[i].st_value) {
			if (in_load_libraries(&strtab[symtab[i].st_name])) {
				add_library_symbol(&lte, i, &library_symbols);
			}
		}
	}

	do_close_load_libraries();
	do_close_elf(&lte);

	return library_symbols;
}
