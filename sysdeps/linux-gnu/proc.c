#include "config.h"
#include "common.h"

#include <sys/types.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* /proc/pid doesn't exist just after the fork, and sometimes `ltrace'
 * couldn't open it to find the executable.  So it may be necessary to
 * have a bit delay
 */

#define	MAX_DELAY	100000	/* 100000 microseconds = 0.1 seconds */

/*
 * Returns a (malloc'd) file name corresponding to a running pid
 */
char *
pid2name(pid_t pid) {
	char proc_exe[1024];

	if (!kill(pid, 0)) {
		int delay = 0;

		sprintf(proc_exe, "/proc/%d/exe", pid);

		while (delay < MAX_DELAY) {
			if (!access(proc_exe, F_OK)) {
				return strdup(proc_exe);
			}
			delay += 1000;	/* 1 milisecond */
		}
	}
	return NULL;
}

static int
find_dynamic_entry_addr(Process *proc, void *pvAddr, int d_tag, void **addr) {
	int i = 0, done = 0;
	ElfW(Dyn) entry;

	debug(DEBUG_FUNCTION, "find_dynamic_entry()");

	if (addr ==	NULL || pvAddr == NULL || d_tag < 0 || d_tag > DT_NUM) {
		return -1;
	}

	while ((!done) && (i < ELF_MAX_SEGMENTS) &&
		(sizeof(entry) == umovebytes(proc, pvAddr, &entry, sizeof(entry))) &&
		(entry.d_tag != DT_NULL)) {
		if (entry.d_tag == d_tag) {
			done = 1;
			*addr = (void *)entry.d_un.d_val;
		}
		pvAddr += sizeof(entry);
		i++;
	}

	if (done) {
		debug(2, "found address: 0x%p in dtag %d\n", *addr, d_tag);
		return 0;
	}
	else {
		debug(2, "Couldn't address for dtag!\n");
		return -1;
	}
}

struct cb_data {
	const char *lib_name;
	struct ltelf *lte;
	ElfW(Addr) addr;
	Process *proc;
};

static void
crawl_linkmap(Process *proc, struct r_debug *dbg, void (*callback)(void *), struct cb_data *data) {
	struct link_map rlm;
	char lib_name[BUFSIZ];
	struct link_map *lm = NULL;

	debug (DEBUG_FUNCTION, "crawl_linkmap()");

	if (!dbg || !dbg->r_map) {
		debug(2, "Debug structure or it's linkmap are NULL!");
		return;
	}

	lm = dbg->r_map;

	while (lm) {
		if (umovebytes(proc, lm, &rlm, sizeof(rlm)) != sizeof(rlm)) {
			debug(2, "Unable to read link map\n");
			return;
		}

		lm = rlm.l_next;
		if (rlm.l_name == NULL) {
			debug(2, "Invalid library name referenced in dynamic linker map\n");
			return;
		}

		umovebytes(proc, rlm.l_name, lib_name, sizeof(lib_name));

		if (lib_name[0] == '\0') {
			debug(2, "Library name is an empty string");
			continue;
		}

		if (callback) {
			debug(2, "Dispatching callback for: %s, Loaded at 0x%x\n", lib_name, rlm.l_addr);
			data->addr = rlm.l_addr;
			data->lib_name = lib_name;
			callback(data);
		}
	}
	return;
}

static struct r_debug *
load_debug_struct(Process *proc) {
	struct r_debug *rdbg = NULL;

	debug(DEBUG_FUNCTION, "load_debug_struct");

	rdbg = malloc(sizeof(*rdbg));
	if (!rdbg) {
		return NULL;
	}

	if (umovebytes(proc, proc->debug, rdbg, sizeof(*rdbg)) != sizeof(*rdbg)) {
		debug(2, "This process does not have a debug structure!\n");
		free(rdbg);
		return NULL;
	}

	return rdbg;
}

static void
linkmap_add_cb(void *data) { //const char *lib_name, ElfW(Addr) addr) {
	int i = 0;
	struct cb_data *lm_add = data;
	struct ltelf lte;
	struct opt_x_t *xptr;

	debug(DEBUG_FUNCTION, "linkmap_add_cb");

	/*
		XXX
		iterate through library[i]'s to see if this lib is in the list.
		if not, add it
	 */
	for(;i < library_num;i++) {
		if (strcmp(library[i], lm_add->lib_name) == 0) {
			/* found it, so its not new */
			return;
		}
	}

	/* new library linked! */
	debug(2, "New libdl loaded library found: %s\n", lm_add->lib_name);

	if (library_num < MAX_LIBRARIES) {
		library[library_num++] = strdup(lm_add->lib_name);
		memset(&lte, 0, sizeof(struct ltelf));
		lte.base_addr = lm_add->addr;
		do_init_elf(&lte, library[library_num-1]);
		/* add bps */
		for (xptr = opt_x; xptr; xptr = xptr->next) {
			if (xptr->found)
				continue;

			GElf_Sym sym;
			GElf_Addr addr;
			struct library_symbol *lsym;

			if (in_load_libraries(xptr->name, &lte, 1, &sym)) {
				debug(2, "found symbol %s @ %lx, adding it.", xptr->name, sym.st_value);
				addr = sym.st_value;
				add_library_symbol(addr, xptr->name, &library_symbols, LS_TOPLT_NONE, 0);
				xptr->found = 1;
				insert_breakpoint(lm_add->proc, sym2addr(lm_add->proc, library_symbols), library_symbols);
			}
		}
		do_close_elf(&lte);
	}
}

void
arch_check_dbg(Process *proc) {
	struct r_debug *dbg = NULL;
	struct cb_data data;

	debug(DEBUG_FUNCTION, "arch_check_dbg");

	if (!(dbg = load_debug_struct(proc))) {
		debug(2, "Unable to load debug structure!");
		return;
	}

	if (dbg->r_state == RT_CONSISTENT) {
		debug(2, "Linkmap is now consistent");
		if (proc->debug_state == RT_ADD) {
			debug(2, "Adding DSO to linkmap");
			data.proc = proc;
			crawl_linkmap(proc, dbg, linkmap_add_cb, &data);
		} else if (proc->debug_state == RT_DELETE) {
			debug(2, "Removing DSO from linkmap");
		} else {
			debug(2, "Unexpected debug state!");
		}
	}

	proc->debug_state = dbg->r_state;

	return;
}

#if 0

/*(XXX TODO)

	 support removal of libraries loaded with dlsym:
		- should be fairly easy, but some refactoring needed
		- create a mapping between libraries and their breakpoints
		- once a library "disappears" form the linkmap, remove each breakpoint
*/

static void
check_still_there(void *data) {
	debug(DEBUG_FUNCTION, "check_still_there");

	/* check if lib found in link map matches the library[i] we are searching
		 for */
}

static void
linkmap_remove() {
	int i = 0;
	debug(DEBUG_FUNCTION, "linkmap_remove");
	/*
		 XXX
		 for each library[i], see if its still in the linkmap. 
	 */

	data.lib_name2find = lib_name;

	/* store some search state in data, too */

	for(;i < library_num;i++) {
		crawl_linkmap(proc, lte, debug struct, check_still_there, data); 
	}
}

#endif

static void
hook_libdl_cb(void *data) { //const char *lib_name, ElfW(Addr) addr) {
	struct cb_data *hook_data = data;
	const char *lib_name = NULL;
	ElfW(Addr) addr;
	struct ltelf *lte = NULL;

	debug(DEBUG_FUNCTION, "add_library_cb");

	if (!data) {
		debug(2, "No callback data");
		return;
	}

	lib_name = hook_data->lib_name;
	addr = hook_data->addr;
	lte = hook_data->lte;

	if (library_num < MAX_LIBRARIES) {
		library[library_num++] = strdup(lib_name);
		lte[library_num].base_addr = addr;
	}
	else {
		fprintf (stderr, "MAX LIBS REACHED\n");
		exit(EXIT_FAILURE);
	}
}

int
linkmap_init(Process *proc, struct ltelf *lte) {
	void *dbg_addr = NULL;
	struct r_debug *rdbg = NULL;
	struct cb_data data;

	debug(DEBUG_FUNCTION, "linkmap_init()");

	if (find_dynamic_entry_addr(proc, (void *)lte->dyn_addr, DT_DEBUG, &dbg_addr) == -1) {
		debug(2, "Couldn't find debug structure!");
		return -1;
	}

	proc->debug = dbg_addr;

	if (!(rdbg = load_debug_struct(proc))) {
		debug(2, "No debug structure or no memory to allocate one!");
		return -1;
	}

	data.lte = lte;

	add_library_symbol(rdbg->r_brk, "", &library_symbols, LS_TOPLT_NONE, 0);
	insert_breakpoint(proc, sym2addr(proc, library_symbols), library_symbols);

	crawl_linkmap(proc, rdbg, hook_libdl_cb, &data);

	free(rdbg);
	return;
}
