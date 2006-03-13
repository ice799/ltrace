#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ltrace.h"
#include "elf.h"
#include "debug.h"
#include "options.h"

static void do_init_elf(struct ltelf *lte, const char *filename);
static void do_close_elf(struct ltelf *lte);
static void add_library_symbol(GElf_Addr addr, const char *name,
			       struct library_symbol **library_symbolspp,
			       int use_elf_plt2addr, int is_weak);
static int in_load_libraries(const char *name, struct ltelf *lte);
static GElf_Addr elf_plt2addr(struct ltelf *ltc, void *addr);

extern char *PLTs_initialized_by_here;

static void do_init_elf(struct ltelf *lte, const char *filename)
{
	int i;
	GElf_Addr relplt_addr = 0;
	size_t relplt_size = 0;

	debug(1, "Reading ELF from %s...", filename);

	memset(lte, 0, sizeof(*lte));
	lte->fd = open(filename, O_RDONLY);
	if (lte->fd == -1)
		error(EXIT_FAILURE, errno, "Can't open \"%s\"", filename);

#ifdef HAVE_ELF_C_READ_MMAP
	lte->elf = elf_begin(lte->fd, ELF_C_READ_MMAP, NULL);
#else
	lte->elf = elf_begin(lte->fd, ELF_C_READ, NULL);
#endif

	if (lte->elf == NULL || elf_kind(lte->elf) != ELF_K_ELF)
		error(EXIT_FAILURE, 0, "Can't open ELF file \"%s\"", filename);

	if (gelf_getehdr(lte->elf, &lte->ehdr) == NULL)
		error(EXIT_FAILURE, 0, "Can't read ELF header of \"%s\"",
		      filename);

	if (lte->ehdr.e_type != ET_EXEC && lte->ehdr.e_type != ET_DYN)
		error(EXIT_FAILURE, 0,
		      "\"%s\" is not an ELF executable nor shared library",
		      filename);

	if ((lte->ehdr.e_ident[EI_CLASS] != LT_ELFCLASS
	     || lte->ehdr.e_machine != LT_ELF_MACHINE)
#ifdef LT_ELF_MACHINE2
	    && (lte->ehdr.e_ident[EI_CLASS] != LT_ELFCLASS2
		|| lte->ehdr.e_machine != LT_ELF_MACHINE2)
#endif
#ifdef LT_ELF_MACHINE3
	    && (lte->ehdr.e_ident[EI_CLASS] != LT_ELFCLASS3
		|| lte->ehdr.e_machine != LT_ELF_MACHINE3)
#endif
	    )
		error(EXIT_FAILURE, 0,
		      "\"%s\" is ELF from incompatible architecture", filename);

	for (i = 1; i < lte->ehdr.e_shnum; ++i) {
		Elf_Scn *scn;
		GElf_Shdr shdr;
		const char *name;

		scn = elf_getscn(lte->elf, i);
		if (scn == NULL || gelf_getshdr(scn, &shdr) == NULL)
			error(EXIT_FAILURE, 0,
			      "Couldn't get section header from \"%s\"",
			      filename);

		name = elf_strptr(lte->elf, lte->ehdr.e_shstrndx, shdr.sh_name);
		if (name == NULL)
			error(EXIT_FAILURE, 0,
			      "Couldn't get section header from \"%s\"",
			      filename);

		if (shdr.sh_type == SHT_SYMTAB) {
			Elf_Data *data;

			lte->symtab = elf_getdata(scn, NULL);
			lte->symtab_count = shdr.sh_size / shdr.sh_entsize;
			if ((lte->symtab == NULL
			     || elf_getdata(scn, lte->symtab) != NULL)
			    && opt_x != NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .symtab data from \"%s\"",
				      filename);

			scn = elf_getscn(lte->elf, shdr.sh_link);
			if (scn == NULL || gelf_getshdr(scn, &shdr) == NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get section header from \"%s\"",
				      filename);

			data = elf_getdata(scn, NULL);
			if (data == NULL || elf_getdata(scn, data) != NULL
			    || shdr.sh_size != data->d_size || data->d_off)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .strtab data from \"%s\"",
				      filename);

			lte->strtab = data->d_buf;
		} else if (shdr.sh_type == SHT_DYNSYM) {
			Elf_Data *data;

			lte->dynsym = elf_getdata(scn, NULL);
			lte->dynsym_count = shdr.sh_size / shdr.sh_entsize;
			if (lte->dynsym == NULL
			    || elf_getdata(scn, lte->dynsym) != NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .dynsym data from \"%s\"",
				      filename);

			scn = elf_getscn(lte->elf, shdr.sh_link);
			if (scn == NULL || gelf_getshdr(scn, &shdr) == NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get section header from \"%s\"",
				      filename);

			data = elf_getdata(scn, NULL);
			if (data == NULL || elf_getdata(scn, data) != NULL
			    || shdr.sh_size != data->d_size || data->d_off)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .dynstr data from \"%s\"",
				      filename);

			lte->dynstr = data->d_buf;
		} else if (shdr.sh_type == SHT_DYNAMIC) {
			Elf_Data *data;
			size_t j;

			data = elf_getdata(scn, NULL);
			if (data == NULL || elf_getdata(scn, data) != NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .dynamic data from \"%s\"",
				      filename);

			for (j = 0; j < shdr.sh_size / shdr.sh_entsize; ++j) {
				GElf_Dyn dyn;

				if (gelf_getdyn(data, j, &dyn) == NULL)
					error(EXIT_FAILURE, 0,
					      "Couldn't get .dynamic data from \"%s\"",
					      filename);

				if (dyn.d_tag == DT_JMPREL)
					relplt_addr = dyn.d_un.d_ptr;
				else if (dyn.d_tag == DT_PLTRELSZ)
					relplt_size = dyn.d_un.d_val;
			}
		} else if (shdr.sh_type == SHT_HASH) {
			Elf_Data *data;
			size_t j;

			data = elf_getdata(scn, NULL);
			if (data == NULL || elf_getdata(scn, data) != NULL
			    || data->d_off || data->d_size != shdr.sh_size)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .hash data from \"%s\"",
				      filename);

			if (shdr.sh_entsize == 4) {
				/* Standard conforming ELF.  */
				if (data->d_type != ELF_T_WORD)
					error(EXIT_FAILURE, 0,
					      "Couldn't get .hash data from \"%s\"",
					      filename);
				lte->hash = (Elf32_Word *) data->d_buf;
			} else if (shdr.sh_entsize == 8) {
				/* Alpha or s390x.  */
				Elf32_Word *dst, *src;
				size_t hash_count = data->d_size / 8;

				lte->hash = (Elf32_Word *)
				    malloc(hash_count * sizeof(Elf32_Word));
				if (lte->hash == NULL)
					error(EXIT_FAILURE, 0,
					      "Couldn't convert .hash section from \"%s\"",
					      filename);
				lte->hash_malloced = 1;
				dst = lte->hash;
				src = (Elf32_Word *) data->d_buf;
				if ((data->d_type == ELF_T_WORD
				     && __BYTE_ORDER == __BIG_ENDIAN)
				    || (data->d_type == ELF_T_XWORD
					&& lte->ehdr.e_ident[EI_DATA] ==
					ELFDATA2MSB))
					++src;
				for (j = 0; j < hash_count; ++j, src += 2)
					*dst++ = *src;
			} else
				error(EXIT_FAILURE, 0,
				      "Unknown .hash sh_entsize in \"%s\"",
				      filename);
		} else if (shdr.sh_type == SHT_PROGBITS
			   || shdr.sh_type == SHT_NOBITS) {
			if (strcmp(name, ".plt") == 0) {
				lte->plt_addr = shdr.sh_addr;
				lte->plt_size = shdr.sh_size;
			} else if (strcmp(name, ".opd") == 0) {
				lte->opd_addr = (GElf_Addr *) shdr.sh_addr;
				lte->opd_size = shdr.sh_size;
				lte->opd = elf_rawdata(scn, NULL);
			}
		}
	}

	if (lte->dynsym == NULL || lte->dynstr == NULL)
		error(EXIT_FAILURE, 0,
		      "Couldn't find .dynsym or .dynstr in \"%s\"", filename);

	if (!relplt_addr || !lte->plt_addr) {
		debug(1, "%s has no PLT relocations", filename);
		lte->relplt = NULL;
		lte->relplt_count = 0;
	} else {
		for (i = 1; i < lte->ehdr.e_shnum; ++i) {
			Elf_Scn *scn;
			GElf_Shdr shdr;

			scn = elf_getscn(lte->elf, i);
			if (scn == NULL || gelf_getshdr(scn, &shdr) == NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get section header from \"%s\"",
				      filename);
			if (shdr.sh_addr == relplt_addr
			    && shdr.sh_size == relplt_size) {
				lte->relplt = elf_getdata(scn, NULL);
				lte->relplt_count =
				    shdr.sh_size / shdr.sh_entsize;
				if (lte->relplt == NULL
				    || elf_getdata(scn, lte->relplt) != NULL)
					error(EXIT_FAILURE, 0,
					      "Couldn't get .rel*.plt data from \"%s\"",
					      filename);
				break;
			}
		}

		if (i == lte->ehdr.e_shnum)
			error(EXIT_FAILURE, 0,
			      "Couldn't find .rel*.plt section in \"%s\"",
			      filename);

		debug(1, "%s %zd PLT relocations", filename, lte->relplt_count);
	}
}

static void do_close_elf(struct ltelf *lte)
{
	if (lte->hash_malloced)
		free((char *)lte->hash);
	elf_end(lte->elf);
	close(lte->fd);
}

static void
add_library_symbol(GElf_Addr addr, const char *name,
		   struct library_symbol **library_symbolspp,
		   int use_elf_plt2addr, int is_weak)
{
	struct library_symbol *s;
	s = malloc(sizeof(struct library_symbol) + strlen(name) + 1);
	if (s == NULL)
		error(EXIT_FAILURE, errno, "add_library_symbol failed");

	s->needs_init = 1;
	s->is_weak = is_weak;
	s->static_plt2addr = use_elf_plt2addr;
	s->next = *library_symbolspp;
	s->enter_addr = (void *)(uintptr_t) addr;
	s->brkpnt = NULL;
	s->name = (char *)(s + 1);
	strcpy(s->name, name);
	*library_symbolspp = s;

	debug(2, "addr: %p, symbol: \"%s\"", (void *)(uintptr_t) addr, name);
}

static int in_load_libraries(const char *name, struct ltelf *lte)
{
	size_t i;
	unsigned long hash;

	if (!library_num)
		return 1;

	hash = elf_hash(name);
	for (i = 1; i <= library_num; ++i) {
		Elf32_Word nbuckets, symndx;
		Elf32_Word *buckets, *chain;

		if (lte[i].hash == NULL)
			continue;

		nbuckets = lte[i].hash[0];
		buckets = &lte[i].hash[2];
		chain = &lte[i].hash[2 + nbuckets];
		for (symndx = buckets[hash % nbuckets];
		     symndx != STN_UNDEF; symndx = chain[symndx]) {
			GElf_Sym sym;

			if (gelf_getsym(lte[i].dynsym, symndx, &sym) == NULL)
				error(EXIT_FAILURE, 0,
				      "Couldn't get symbol from .dynsym");

			if (sym.st_value != 0
			    && sym.st_shndx != SHN_UNDEF
			    && strcmp(name, lte[i].dynstr + sym.st_name) == 0)
				return 1;
		}
	}
	return 0;
}

static GElf_Addr elf_plt2addr(struct ltelf *lte, void *addr)
{
	long base;
	long offset;
	GElf_Addr ret_val;

	if (!lte->opd)
		return (GElf_Addr) addr;

	base = (long)lte->opd->d_buf;
	offset = (long)addr - (long)lte->opd_addr;
	if (offset > lte->opd_size)
		error(EXIT_FAILURE, 0, "static plt not in .opd");

	ret_val = (GElf_Addr) * (long *)(base + offset);
	return ret_val;
}

struct library_symbol *read_elf(struct process *proc)
{
	struct library_symbol *library_symbols = NULL;
	struct ltelf lte[MAX_LIBRARY + 1];
	size_t i;
	struct opt_x_t *xptr;
	struct library_symbol **lib_tail = NULL;
	struct opt_x_t *main_cheat;
	int exit_out = 0;

	elf_version(EV_CURRENT);

	do_init_elf(lte, proc->filename);
	proc->e_machine = lte->ehdr.e_machine;
	for (i = 0; i < library_num; ++i)
		do_init_elf(&lte[i + 1], library[i]);

	for (i = 0; i < lte->relplt_count; ++i) {
		GElf_Rel rel;
		GElf_Rela rela;
		GElf_Sym sym;
		GElf_Addr addr;
		void *ret;
		const char *name;

		if (lte->relplt->d_type == ELF_T_REL) {
			ret = gelf_getrel(lte->relplt, i, &rel);
			rela.r_offset = rel.r_offset;
			rela.r_info = rel.r_info;
			rela.r_addend = 0;
		} else
			ret = gelf_getrela(lte->relplt, i, &rela);

		if (ret == NULL
		    || ELF64_R_SYM(rela.r_info) >= lte->dynsym_count
		    || gelf_getsym(lte->dynsym, ELF64_R_SYM(rela.r_info),
				   &sym) == NULL)
			error(EXIT_FAILURE, 0,
			      "Couldn't get relocation from \"%s\"",
			      proc->filename);

		if (!sym.st_value && PLTs_initialized_by_here)
			proc->need_to_reinitialize_breakpoints = 1;

		name = lte->dynstr + sym.st_name;
		if (in_load_libraries(name, lte)) {
			addr = arch_plt_sym_val(lte, i, &rela);
			add_library_symbol(addr, name, &library_symbols, 0,
					   ELF64_ST_BIND(sym.st_info) != 0);
			if (!lib_tail)
				lib_tail = &(library_symbols->next);
		}
	}

	if (proc->need_to_reinitialize_breakpoints) {
		/* Add "PLTs_initialized_by_here" to opt_x list, if not already there. */
		main_cheat = (struct opt_e_t *)malloc(sizeof(struct opt_e_t));
		if (main_cheat == NULL)
			error(EXIT_FAILURE, 0, "Couldn allocate memory");
		main_cheat->next = opt_x;
		main_cheat->name = PLTs_initialized_by_here;

		for (xptr = opt_x; xptr; xptr = xptr->next)
			if (strcmp(xptr->name, PLTs_initialized_by_here) == 0
			    && main_cheat) {
				free(main_cheat);
				main_cheat = NULL;
				break;
			}
		if (main_cheat)
			opt_x = main_cheat;
	}

	for (i = 0; i < lte->symtab_count; ++i) {
		GElf_Sym sym;
		GElf_Addr addr;
		const char *name;

		if (gelf_getsym(lte->symtab, i, &sym) == NULL)
			error(EXIT_FAILURE, 0,
			      "Couldn't get symbol from \"%s\"",
			      proc->filename);

		name = lte->strtab + sym.st_name;
		addr = sym.st_value;
		if (!addr)
			continue;

		for (xptr = opt_x; xptr; xptr = xptr->next)
			if (xptr->name && strcmp(xptr->name, name) == 0) {
				/* FIXME: Should be able to use &library_symbols as above.  But
				   when you do, none of the real library symbols cause breaks. */
				add_library_symbol(elf_plt2addr
						   (lte, (void *)addr), name,
						   lib_tail, 1, 0);
				xptr->found = 1;
				break;
			}
	}
	for (xptr = opt_x; xptr; xptr = xptr->next)
		if ( ! xptr->found) {
			char *badthing = "WARNING";
			if (E_ENTRY_NAME && strcmp(xptr->name, E_ENTRY_NAME)) {
				badthing = "ERROR";
				exit_out = 1;
			}
			fprintf (stderr,
				 "%s: Couldn't find symbol \"%s\" in file \"%s\"\n",
			badthing, xptr->name, proc->filename);
		}
	if (exit_out) {
		exit (1);
	}

	for (i = 0; i < library_num + 1; ++i)
		do_close_elf(&lte[i]);

	return library_symbols;
}
