/*
 * This file contains functions specific to ELF binaries
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/elf.h>
#include <sys/mman.h>
#include <string.h>

#include "elf.h"
#include "ltrace.h"
#include "symbols.h"
#include "output.h"

int read_elf(const char *filename)
{
	struct stat sbuf;
	int fd;
	void * addr;
	struct elf32_hdr * hdr;
	Elf32_Shdr * shdr;
	struct elf32_sym * symtab = NULL;
	int i;
	char * strtab = NULL;
	u_long symtab_len = 0;

	if (opt_d>0) {
		send_line("Reading symbol table from %s...", filename);
	}

	fd = open(filename, O_RDONLY);
	if (fd==-1) {
		fprintf(stderr, "Can't open \"%s\": %s\n", filename, sys_errlist[errno]);
		exit(1);
	}
	if (fstat(fd, &sbuf)==-1) {
		fprintf(stderr, "Can't stat \"%s\": %s\n", filename, sys_errlist[errno]);
		exit(1);
	}
	if (sbuf.st_size < sizeof(struct elf32_hdr)) {
		fprintf(stderr, "\"%s\" is not an ELF object\n", filename);
		exit(1);
	}
	addr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (addr==(void*)-1) {
		fprintf(stderr, "Can't mmap \"%s\": %s\n", filename, sys_errlist[errno]);
		exit(1);
	}
	hdr = addr;
	if (strncmp(hdr->e_ident, ELFMAG, SELFMAG)) {
		fprintf(stderr, "\"%s\" is not an ELF object\n", filename);
		exit(1);
	}
	for(i=0; i<hdr->e_shnum; i++) {
		shdr = addr + hdr->e_shoff + i*hdr->e_shentsize;
		if (shdr->sh_type == SHT_DYNSYM) {
			if (!symtab) {
#if 0
				symtab = (struct elf32_sym *)shdr->sh_addr;
#else
				symtab = (struct elf32_sym *)(addr + shdr->sh_offset);
#endif
				symtab_len = shdr->sh_size;
			}
		}
		if (shdr->sh_type == SHT_STRTAB) {
			if (!strtab) {
				strtab = (char *)(addr + shdr->sh_offset);
			}
		}
	}
	if (opt_d>1) {
		send_line("symtab: 0x%08x", (unsigned)symtab);
		send_line("symtab_len: %lu", symtab_len);
		send_line("strtab: 0x%08x", (unsigned)strtab);
	}
	if (!symtab) {
		return 0;
	}
	for(i=0; i<symtab_len/sizeof(struct elf32_sym); i++) {
		if (!((symtab+i)->st_shndx) && (symtab+i)->st_value) {
			struct library_symbol * tmp = library_symbols;

			library_symbols = malloc(sizeof(struct library_symbol));
			if (!library_symbols) {
				perror("malloc");
				exit(1);
			}
			library_symbols->sbp.addr = ((symtab+i)->st_value);
			library_symbols->name = strtab+(symtab+i)->st_name;
			library_symbols->next = tmp;
			if (opt_d>1) {
				send_line("addr: 0x%08x, symbol: \"%s\"",
					(unsigned)((symtab+i)->st_value),
					(strtab+(symtab+i)->st_name));
			}
		}
	}
	return 1;
}

