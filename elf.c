#include "config.h"

#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

void do_init_elf(struct ltelf *lte, const char *filename);
void do_close_elf(struct ltelf *lte);
void add_library_symbol(GElf_Addr addr, const char *name,
			       struct library_symbol **library_symbolspp,
			       enum toplt type_of_plt, int is_weak);
int in_load_libraries(const char *name, struct ltelf *lte, size_t count, GElf_Sym *sym);
static GElf_Addr opd2addr(struct ltelf *ltc, GElf_Addr addr);

struct library_symbol *library_symbols;
struct ltelf main_lte;

#ifdef PLT_REINITALISATION_BP
extern char *PLTs_initialized_by_here;
#endif

int libdl_hooked = 0;

void
do_init_elf(struct ltelf *lte, const char *filename) {
	int i;
	GElf_Addr relplt_addr = 0;
	size_t relplt_size = 0;

	debug(DEBUG_FUNCTION, "do_init_elf(filename=%s)", filename);
	debug(1, "Reading ELF from %s...", filename);

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
      
      lte->dyn_addr = shdr.sh_addr;
      lte->dyn_sz = shdr.sh_size;

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
#ifdef __mips__
/**
  MIPS ABI Supplement:

  DT_PLTGOT This member holds the address of the .got section.

  DT_MIPS_SYMTABNO This member holds the number of entries in the
  .dynsym section.

  DT_MIPS_LOCAL_GOTNO This member holds the number of local global
  offset table entries.

  DT_MIPS_GOTSYM This member holds the index of the first dyamic
  symbol table entry that corresponds to an entry in the gobal offset
  table.

 */
				if(dyn.d_tag==DT_PLTGOT){
					lte->pltgot_addr=dyn.d_un.d_ptr;
				}
				if(dyn.d_tag==DT_MIPS_LOCAL_GOTNO){
					lte->mips_local_gotno=dyn.d_un.d_val;
				}
				if(dyn.d_tag==DT_MIPS_GOTSYM){
					lte->mips_gotsym=dyn.d_un.d_val;
				}
#endif // __mips__
				if (dyn.d_tag == DT_JMPREL)
					relplt_addr = dyn.d_un.d_ptr;
				else if (dyn.d_tag == DT_PLTRELSZ)
					relplt_size = dyn.d_un.d_val;
			}
		} else if (shdr.sh_type == SHT_HASH) {
			Elf_Data *data;
			size_t j;

			lte->hash_type = SHT_HASH;

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
				lte->lte_flags |= LTE_HASH_MALLOCED;
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
		} else if (shdr.sh_type == SHT_GNU_HASH
			   && lte->hash == NULL) {
			Elf_Data *data;

			lte->hash_type = SHT_GNU_HASH;

			if (shdr.sh_entsize != 0
			    && shdr.sh_entsize != 4) {
				error(EXIT_FAILURE, 0,
				      ".gnu.hash sh_entsize in \"%s\" should be 4, but is %llu",
				      filename, shdr.sh_entsize);
			}

			data = elf_getdata(scn, NULL);
			if (data == NULL || elf_getdata(scn, data) != NULL
			    || data->d_off || data->d_size != shdr.sh_size)
				error(EXIT_FAILURE, 0,
				      "Couldn't get .gnu.hash data from \"%s\"",
				      filename);

			lte->hash = (Elf32_Word *) data->d_buf;
		} else if (shdr.sh_type == SHT_PROGBITS
			   || shdr.sh_type == SHT_NOBITS) {
			if (strcmp(name, ".plt") == 0) {
				lte->plt_addr = shdr.sh_addr;
				lte->plt_size = shdr.sh_size;
				if (shdr.sh_flags & SHF_EXECINSTR) {
					lte->lte_flags |= LTE_PLT_EXECUTABLE;
				}
			}
#ifdef ARCH_SUPPORTS_OPD
			else if (strcmp(name, ".opd") == 0) {
				lte->opd_addr = (GElf_Addr *) (long) shdr.sh_addr;
				lte->opd_size = shdr.sh_size;
				lte->opd = elf_rawdata(scn, NULL);
			}
#endif
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

void
do_close_elf(struct ltelf *lte) {
	debug(DEBUG_FUNCTION, "do_close_elf()");
	if (lte->lte_flags & LTE_HASH_MALLOCED)
		free((char *)lte->hash);
	elf_end(lte->elf);
	close(lte->fd);
}

void
add_library_symbol(GElf_Addr addr, const char *name,
		   struct library_symbol **library_symbolspp,
		   enum toplt type_of_plt, int is_weak) {
	struct library_symbol *s;

	debug(DEBUG_FUNCTION, "add_library_symbol()");

	s = malloc(sizeof(struct library_symbol) + strlen(name) + 1);
	if (s == NULL)
		error(EXIT_FAILURE, errno, "add_library_symbol failed");

	s->needs_init = 1;
	s->is_weak = is_weak;
	s->plt_type = type_of_plt;
	s->next = *library_symbolspp;
	s->enter_addr = (void *)(uintptr_t) addr;
	s->name = (char *)(s + 1);
	strcpy(s->name, name);
	*library_symbolspp = s;

	debug(2, "addr: %p, symbol: \"%s\"", (void *)(uintptr_t) addr, name);
}

/* stolen from elfutils-0.123 */
static unsigned long
private_elf_gnu_hash(const char *name) {
	unsigned long h = 5381;
	const unsigned char *string = (const unsigned char *)name;
	unsigned char c;
	for (c = *string; c; c = *++string)
		h = h * 33 + c;
	return h & 0xffffffff;
}

int
in_load_libraries(const char *name, struct ltelf *lte, size_t count, GElf_Sym *sym) {
	size_t i;
	unsigned long hash;
	unsigned long gnu_hash;

	if (!count)
		return 1;

	hash = elf_hash((const unsigned char *)name);
	gnu_hash = private_elf_gnu_hash(name);
	for (i = 0; i < count; ++i) {
		if (lte[i].hash == NULL)
			continue;

		if (lte[i].hash_type == SHT_GNU_HASH) {
			Elf32_Word * hashbase = lte[i].hash;
			Elf32_Word nbuckets = *hashbase++;
			Elf32_Word symbias = *hashbase++;
			Elf32_Word bitmask_nwords = *hashbase++;
			Elf32_Word * buckets;
			Elf32_Word * chain_zero;
			Elf32_Word bucket;

			// +1 for skipped `shift'
			hashbase += lte[i].ehdr.e_ident[EI_CLASS] * bitmask_nwords + 1;
			buckets = hashbase;
			hashbase += nbuckets;
			chain_zero = hashbase - symbias;
			bucket = buckets[gnu_hash % nbuckets];

			if (bucket != 0) {
				const Elf32_Word *hasharr = &chain_zero[bucket];
				do
					if ((*hasharr & ~1u) == (gnu_hash & ~1u)) {
						int symidx = hasharr - chain_zero;
						GElf_Sym tmp_sym;
						GElf_Sym *tmp;

						tmp = (sym) ? (sym) : (&tmp_sym);

						if (gelf_getsym(lte[i].dynsym, symidx, tmp) == NULL)
							error(EXIT_FAILURE, 0, "Couldn't get symbol from .dynsym");
						else {
							tmp->st_value += lte[i].base_addr;
							debug(2, "symbol found: %s, %zd, %lx", name, i, tmp->st_value);
						}
						if (tmp->st_value != 0
						    && tmp->st_shndx != SHN_UNDEF
						    && strcmp(name, lte[i].dynstr + tmp->st_name) == 0)
							return 1;
					}
				while ((*hasharr++ & 1u) == 0);
			}
		} else {
			Elf32_Word nbuckets, symndx;
			Elf32_Word *buckets, *chain;
			nbuckets = lte[i].hash[0];
			buckets = &lte[i].hash[2];
			chain = &lte[i].hash[2 + nbuckets];
			for (symndx = buckets[hash % nbuckets];
			     symndx != STN_UNDEF; symndx = chain[symndx]) {
				GElf_Sym tmp_sym;
				GElf_Sym *tmp;

				tmp = (sym) ? (sym) : (&tmp_sym);

				if (gelf_getsym(lte[i].dynsym, symndx, tmp) == NULL)
					error(EXIT_FAILURE, 0,
					      "Couldn't get symbol from .dynsym");
				else {
					tmp->st_value += lte[i].base_addr;
					debug(2, "symbol found: %s, %zd, %lx", name, i, tmp->st_value);
				}

				if (tmp->st_value != 0
				    && tmp->st_shndx != SHN_UNDEF
				    && strcmp(name, lte[i].dynstr + tmp->st_name) == 0)
					return 1;
			}
		}
	}
	return 0;
}

static GElf_Addr
opd2addr(struct ltelf *lte, GElf_Addr addr) {
#ifdef ARCH_SUPPORTS_OPD
	unsigned long base, offset;

	if (!lte->opd)
		return addr;

	base = (unsigned long)lte->opd->d_buf;
	offset = (unsigned long)addr - (unsigned long)lte->opd_addr;
	if (offset > lte->opd_size)
		error(EXIT_FAILURE, 0, "static plt not in .opd");

	return *(GElf_Addr*)(base + offset);
#else //!ARCH_SUPPORTS_OPD
	return addr;
#endif
}

struct library_symbol *
read_elf(Process *proc) {
	struct ltelf lte[MAX_LIBRARIES + 1];
	size_t i;
	struct opt_x_t *xptr;
	struct library_symbol **lib_tail = NULL;
	int exit_out = 0;
	int count  = 0;

	debug(DEBUG_FUNCTION, "read_elf(file=%s)", proc->filename);

	memset(lte, 0, sizeof(lte));

	elf_version(EV_CURRENT);

	do_init_elf(lte, proc->filename);

	memcpy(&main_lte, lte, sizeof(struct ltelf));

	if (opt_p && opt_p->pid > 0) {
		linkmap_init(proc, lte);
		libdl_hooked = 1;
	}

	proc->e_machine = lte->ehdr.e_machine;

	for (i = 0; i < library_num; ++i) {
		do_init_elf(&lte[i + 1], library[i]);
	}

	if (!options.no_plt) {
#ifdef __mips__
		// MIPS doesn't use the PLT and the GOT entries get changed
		// on startup.
		proc->need_to_reinitialize_breakpoints = 1;
		for(i=lte->mips_gotsym; i<lte->dynsym_count;i++){
			GElf_Sym sym;
			const char *name;
			GElf_Addr addr = arch_plt_sym_val(lte, i, 0);
			if (gelf_getsym(lte->dynsym, i, &sym) == NULL){
				error(EXIT_FAILURE, 0,
						"Couldn't get relocation from \"%s\"",
						proc->filename);
			}
			name=lte->dynstr+sym.st_name;
			if(ELF64_ST_TYPE(sym.st_info) != STT_FUNC){
				debug(2,"sym %s not a function",name);
				continue;
			}
			add_library_symbol(addr, name, &library_symbols, 0,
					ELF64_ST_BIND(sym.st_info) != 0);
			if (!lib_tail)
				lib_tail = &(library_symbols->next);
		}
#else
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

#ifdef PLT_REINITALISATION_BP
			if (!sym.st_value && PLTs_initialized_by_here)
				proc->need_to_reinitialize_breakpoints = 1;
#endif

			name = lte->dynstr + sym.st_name;
			count = library_num ? library_num+1 : 0;
			if (in_load_libraries(name, lte, count, NULL)) {
				addr = arch_plt_sym_val(lte, i, &rela);
				add_library_symbol(addr, name, &library_symbols,
						(PLTS_ARE_EXECUTABLE(lte)
						 ?	LS_TOPLT_EXEC : LS_TOPLT_POINT),
						ELF64_ST_BIND(sym.st_info) == STB_WEAK);
				if (!lib_tail)
					lib_tail = &(library_symbols->next);
			}
		}
#endif // !__mips__
#ifdef PLT_REINITALISATION_BP
		struct opt_x_t *main_cheat;

		if (proc->need_to_reinitialize_breakpoints) {
			/* Add "PLTs_initialized_by_here" to opt_x list, if not
				 already there. */
			main_cheat = (struct opt_x_t *)malloc(sizeof(struct opt_x_t));
			if (main_cheat == NULL)
				error(EXIT_FAILURE, 0, "Couldn't allocate memory");
			main_cheat->next = opt_x;
			main_cheat->found = 0;
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
#endif
	} else {
		lib_tail = &library_symbols;
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
				add_library_symbol(opd2addr(lte, addr),
						   name, lib_tail, LS_TOPLT_NONE, 0);
				xptr->found = 1;
				break;
			}
	}

	int found_count = 0;

	for (xptr = opt_x; xptr; xptr = xptr->next) {
		if (xptr->found)
			continue;

		GElf_Sym sym;
		GElf_Addr addr;
		if (in_load_libraries(xptr->name, lte, library_num+1, &sym)) {
			debug(2, "found symbol %s @ %lx, adding it.", xptr->name, sym.st_value);
			addr = sym.st_value;
			add_library_symbol(addr, xptr->name, lib_tail, LS_TOPLT_NONE, 0);
			xptr->found = 1;
			found_count++;
		}
		if (found_count == opt_x_cnt){
			debug(2, "done, found everything: %d\n", found_count);
			break;
		}
	}

	for (xptr = opt_x; xptr; xptr = xptr->next)
		if ( ! xptr->found) {
			char *badthing = "WARNING";
#ifdef PLT_REINITALISATION_BP
			if (strcmp(xptr->name, PLTs_initialized_by_here) == 0) {
				if (lte->ehdr.e_entry) {
					add_library_symbol (
						opd2addr (lte, lte->ehdr.e_entry),
						PLTs_initialized_by_here,
						lib_tail, 1, 0);
					fprintf (stderr, "WARNING: Using e_ent"
						 "ry from elf header (%p) for "
						 "address of \"%s\"\n", (void*)
						 (long) lte->ehdr.e_entry,
						 PLTs_initialized_by_here);
					continue;
				}
				badthing = "ERROR";
				exit_out = 1;
			}
#endif
			fprintf (stderr,
				 "%s: Couldn't find symbol \"%s\" in file \"%s\" assuming it will be loaded by libdl!"
				 "\n", badthing, xptr->name, proc->filename);
		}
	if (exit_out) {
		exit (1);
	}

	for (i = 0; i < library_num + 1; ++i)
		do_close_elf(&lte[i]);

	return library_symbols;
}
