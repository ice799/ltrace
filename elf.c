# include "config.h"

#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <link.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

static void do_init_elf(struct ltelf *lte, const char *filename);
static void do_close_elf(struct ltelf *lte);
static void add_library_symbol(GElf_Addr addr, const char *name,
			       struct library_symbol **library_symbolspp,
			       enum toplt type_of_plt, int is_weak);
static int in_load_libraries(const char *name, struct ltelf *lte);
static GElf_Addr opd2addr(struct ltelf *ltc, GElf_Addr addr);

static struct library_symbol *library_symbols = NULL;

#ifdef PLT_REINITALISATION_BP
extern char *PLTs_initialized_by_here;
#endif

static void
do_init_elf(struct ltelf *lte, const char *filename) {
	int i;
	GElf_Addr relplt_addr = 0;
	size_t relplt_size = 0;

	debug(DEBUG_FUNCTION, "do_init_elf(filename=%s)", filename);
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
        else if (dyn.d_tag == DT_DEBUG) {
          lte->debug_offset = sizeof(GElf_Dyn) * j;
          debug(2, "vma of dynamic: %lx:%lx debug offset: %zd bytes: %zd\n", shdr.sh_addr, shdr.sh_size, j, lte->debug_offset);
        }
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

static void
do_close_elf(struct ltelf *lte) {
	debug(DEBUG_FUNCTION, "do_close_elf()");
	if (lte->lte_flags & LTE_HASH_MALLOCED)
		free((char *)lte->hash);
	elf_end(lte->elf);
	close(lte->fd);
}

static void
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

static int
in_load_libraries(const char *name, struct ltelf *lte) {
	size_t i;
	unsigned long hash;
	unsigned long gnu_hash;

	if (!library_num)
		return 1;

	hash = elf_hash((const unsigned char *)name);
	gnu_hash = private_elf_gnu_hash(name);
	for (i = 1; i <= library_num; ++i) {
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
						GElf_Sym sym;

						if (gelf_getsym(lte[i].dynsym, symidx, &sym) == NULL)
							error(EXIT_FAILURE, 0,
							      "Couldn't get symbol from .dynsym");

						if (sym.st_value != 0
						    && sym.st_shndx != SHN_UNDEF
						    && strcmp(name, lte[i].dynstr + sym.st_name) == 0)
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


static int
find_dynamic_entry(Process *proc, void *pvAddr, Elf64_Sxword d_tag, GElf_Dyn *entry) {
  int i = 0, done = 0;

  debug(DEBUG_FUNCTION, "find_dynamic_entry()");

  if (entry == NULL || pvAddr == NULL || d_tag < 0 || d_tag > DT_NUM) {
    return -1;
  }

  while ((!done) && (i < ELF_MAX_SEGMENTS) && 
      (sizeof(*entry) == umovebytes(proc, pvAddr, entry, sizeof(*entry))) &&
      (entry->d_tag != DT_NULL)) {
    if (entry->d_tag == d_tag) 
      done = 1;
    pvAddr += sizeof(*entry);
    i++;
  }

  if (done) {
    debug(2, "found address: 0x%lx in dtag %ld\n", entry->d_un.d_val, d_tag);
    return(0);
  }
  else {
    debug(2, "Couldn't address for dtag!\n");
    return(-1);
  }
}

static int
check_lib(const char *lib_name) {
  int i = 0;
  for(i = 0; i < library_num; i++)
    if (strcmp(lib_name, library[i]) == 0)
      return 0;

  return 1;
}

void
check_sym_name(const char *sym_name, GElf_Addr sym_addr, unsigned char sym_info) {
  Function *tmp = list_of_functions;
  
  debug(DEBUG_FUNCTION, "check_sym_name()");

  while (tmp != NULL) {
    const char *demangled = my_demangle(sym_name);
    char *found = strchr(demangled, '(');
    int demang_len = 0;
    if (found) {
      demang_len = found - demangled - 1;
    }
    if (strcmp(sym_name, tmp->name) == 0) {
      add_library_symbol(sym_addr, sym_name, &library_symbols,
          LS_TOPLT_NONE,
          ELF64_ST_BIND(sym_info)== STB_WEAK);
      debug(2, "added symbol %s!\n", tmp->name);
    }
    else if (found && strncmp(demangled, tmp->name, demang_len) == 0) {
      add_library_symbol(sym_addr, demangled, &library_symbols,
          LS_TOPLT_NONE,
          ELF64_ST_BIND(sym_info) == STB_WEAK);
      debug(2, "added c++ symbol %s!\n", tmp->name);
    }

    tmp = tmp->next;
  }
}

static int
crawl_symtab(Process *proc, struct link_map *lm, const char *str_tab, GElf_Sym *symtab, Elf64_Xword sym_ent_len) {
  int i = 0, bytes_read = 0;
  GElf_Sym tmp;
  GElf_Sym *tmp_symtab = symtab;
  GElf_Addr sym_addr;
  char sym_name[BUFSIZ];

  debug(DEBUG_FUNCTION, "crawl_symtab(): %p", symtab);

  /* XXX need a better loop invariant!!! */
  while ((i < ELF_SYMTAB_MAX)) {
    if (umovebytes(proc, tmp_symtab, &tmp, sizeof(tmp)) != sizeof(tmp)) {
      debug(2, "Couldn't read symtab entry");
      return -1;
    }

    tmp_symtab = (GElf_Sym *) ((unsigned long) tmp_symtab + (unsigned long) (sym_ent_len));
    i++;

    if (!tmp.st_name) {
      continue;
    }
    if (!tmp.st_size){
      continue;
    }
    if (!(ELF32_ST_TYPE(tmp.st_info) == STT_FUNC)) {
      continue;
    }
    if (!tmp.st_value) {
      continue;
    }

    bytes_read = umovebytes(proc, str_tab + tmp.st_name, sym_name, sizeof(sym_name));

    if (bytes_read < 0) {
      debug(2, "Could not read symbol name\n");
      continue; 
    }
   
    sym_name[bytes_read] = '\0';
    sym_addr = (GElf_Addr)lm->l_addr + tmp.st_value;

    debug(2, "Found symbol %s @ 0x%lx", sym_name, sym_addr);

    check_sym_name(sym_name, sym_addr, tmp.st_info);
  }

  return 0; 
}

static int
crawl_linkmap(Process *proc, struct link_map *lm) {
  GElf_Dyn tmp;
  GElf_Sym *sym_tab = NULL;
  struct link_map rlm;
  const char *str_table = NULL;
  char lib_name[BUFSIZ];
  Elf64_Xword sym_ent_len = 0;

  debug (DEBUG_FUNCTION, "crawl_linkmap()");
  
  while (lm) {
    if (umovebytes(proc, lm, &rlm, sizeof(rlm)) != sizeof(rlm)) {
      debug(2, "Unable to read link map\n");
      return -1;
    }

    lm = rlm.l_next;
    if (rlm.l_name == NULL) {
      debug(2, "Invalid library name referenced in dynamic linker map\n");
      return -1;
    }

    umovebytes(proc, rlm.l_name, lib_name, sizeof(lib_name));

    if (lib_name[0] == '\0') {
      debug(2, "Library name is an empty string");
      continue;
    }

    debug(2, "Object %s, Loaded at 0x%lx\n", lib_name, rlm.l_addr);
    
    if (check_lib(lib_name)) {
      debug(2, "User doesn't care about this library, skipping.");
      continue;
    }

    if (!rlm.l_ld)  {
      debug(2, "Could not find .dynamic section for DSO %s\n", lib_name);

      /* XXX return, really? */
      return -1;
    }

    debug(2, ".dynamic section for library %s is at 0x%p size:%zd\n", lib_name, rlm.l_ld, sizeof(rlm.l_ld));

    if (find_dynamic_entry(proc, (void *) rlm.l_ld, DT_SYMTAB, &tmp) == -1) {
      debug(2, "Couldn't find symtab!\n");
      
      /* XXX look for static symtab? */
      return -1;
    }

    sym_tab = (GElf_Sym *) tmp.d_un.d_ptr;

    debug(2, "symtab: %p, l_addr: %lx", sym_tab, rlm.l_addr);
    /* If the address is less than the base offset, it is probably a relative
       address and we need to adjust sym_tab accordingly */
    if (sym_tab < (Elf64_Sym *) rlm.l_addr)
      sym_tab = (GElf_Sym *) ((unsigned long) sym_tab + rlm.l_addr);

    if (find_dynamic_entry(proc, (void *) rlm.l_ld, DT_SYMENT, &tmp) == -1) {
      debug(2, "Couldn't read symtab entry length!");
      return -1;
    }

    sym_ent_len = tmp.d_un.d_val;

    if (find_dynamic_entry(proc, (void *) rlm.l_ld, DT_STRTAB, &tmp) == -1) {
      debug(2, "No string table found for library %s\n", lib_name);
      return -1;
    }

    str_table = (const char *) tmp.d_un.d_ptr;

    crawl_symtab(proc, &rlm, str_table, sym_tab, sym_ent_len);
  }
  return 0;
}


void
linkmap_init(Process *proc, struct ltelf *lte) {
  GElf_Dyn tmp;
  struct r_debug rdbg, *tdbg = NULL;
  struct link_map *tlink_map = NULL;

  debug(DEBUG_FUNCTION, "linkmap_init()");

  if (find_dynamic_entry(proc, (void *)lte->dyn_addr, DT_DEBUG, &tmp) == -1) {
    debug(2, "Couldn't find debug structure!");
    return;
  }
  
  tdbg = (struct r_debug *) tmp.d_un.d_val;
  /* XXX
     this isn't actually a failure case - we need to set breakpoints on
     dl_debug_state and then re-try this function.
   */
  if (umovebytes(proc, tdbg, &rdbg, sizeof(rdbg)) != sizeof(rdbg)) {
    debug(2, "This process does not have a debug structure!\n");
    return;
  }

  tlink_map = rdbg.r_map;
  crawl_linkmap(proc, tlink_map);

  return;
}

struct library_symbol *
read_elf(Process *proc) {
	struct ltelf lte[MAX_LIBRARIES + 1];
	size_t i;
	struct opt_x_t *xptr;
	struct library_symbol **lib_tail = NULL;
	int exit_out = 0;

	debug(DEBUG_FUNCTION, "read_elf(file=%s)", proc->filename);

	elf_version(EV_CURRENT);

	do_init_elf(lte, proc->filename);

  /* XXX if attaching to pid or something */
  linkmap_init(proc, lte);

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
			if (in_load_libraries(name, lte)) {
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
				 "%s: Couldn't find symbol \"%s\" in file \"%s"
			         "\"\n", badthing, xptr->name, proc->filename);
		}
	if (exit_out) {
		exit (1);
	}

	for (i = 0; i < library_num + 1; ++i)
		do_close_elf(&lte[i]);

	return library_symbols;
}
