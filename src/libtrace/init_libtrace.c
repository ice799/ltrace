#include <fcntl.h>
#include <sys/types.h>
#include <linux/elf.h>
#include <hck/syscall.h>
#include <sys/mman.h>

static int fd;

static char trampoline[] = {
	1, 2, 3, 4,			/* pointer to this pushl:		*/
	0x68, 1, 2, 3, 4,		/* pushl $0x04030201 (symbol index)	*/
	0xff, 0x25, 1, 2, 3, 4		/* jmp *0x04030201			*/
};

struct trampoline_t {
	void * aqui __attribute__ ((packed));
	char tmp1;
	long function_no __attribute__ ((packed));
	char tmp2, tmp3;
	void (*new_func)() __attribute__ ((packed));
};

void ** GOT;		/* Array indexed by 'symbol index' (value pushed) */
			/* This is the address of the real GOT corresponding to this function */
char ** names;		/* names of the functions called */

static void new_func(void);
static void * new_func_ptr = NULL;

static int current_pid;

static void init_libtrace(void)
{
	int i,j;
	struct elf32_sym * symtab = NULL;
	size_t symtab_len = 0;
	char * tmp;
	long buffer[6];
	struct trampoline_t * table;
	unsigned long plt_min=-1, plt_max=0;
	char * strtab = NULL;
	size_t nsymbols;

	if (new_func_ptr) {
		return;
	}
	new_func_ptr = new_func;

	current_pid = _sys_getpid();

	tmp = getenv("LTRACE_FILENAME");
	if (tmp) {
		int fd_old;

		fd_old = _sys_open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd_old<0) {
			_sys_write(2, "Can't open LTRACE_FILENAME\n", 27);
			return;
		}
		fd = _sys_dup2(fd_old, 234);
		_sys_close(fd_old);
	} else {
		fd = _sys_dup2(2, 234);
	}
	if (fd<0) {
		_sys_write(2, "Not enough fd's?\n", 17);
		return;
	}

	tmp = getenv("LTRACE_SYMTAB");
	if (!tmp) {
		_sys_write(fd, "No LTRACE_SYMTAB...\n", 20);
		return;
	}
	symtab = (struct elf32_sym *)atoi(tmp);

	tmp = getenv("LTRACE_SYMTAB_LEN");
	if (!tmp) {
		_sys_write(fd, "No LTRACE_SYMTAB_LEN...\n", 24);
		return;
	}
	symtab_len = atoi(tmp);

	tmp = getenv("LTRACE_STRTAB");
	if (!tmp) {
		_sys_write(fd, "No LTRACE_STRTAB...\n", 20);
		return;
	}
	strtab = (char *)atoi(tmp);

	unsetenv("LD_PRELOAD");
	unsetenv("LTRACE_SYMTAB");
	unsetenv("LTRACE_SYMTAB_LEN");
	unsetenv("LTRACE_STRTAB");

	nsymbols = 0;
	for(i=0; i<symtab_len/sizeof(struct elf32_sym); i++) {
		if (!((symtab+i)->st_shndx) && (symtab+i)->st_value) {
			unsigned got_tmp;
			nsymbols++;
			if ((symtab+i)->st_value < plt_min) {
				plt_min = (symtab+i)->st_value;
			}
			if ((symtab+i)->st_value > plt_max) {
				plt_max = (symtab+i)->st_value;
			}

#if 1
                        got_tmp = (long)*(long *)(((int)((symtab+i)->st_value))+2);

			printf("New symbol: %-16s, JMP: 0x%08x, GOT: 0x%08x\n",
				(strtab+(symtab+i)->st_name),
				(unsigned)((symtab+i)->st_value), got_tmp);
			printf("tpnt = 0x%08lx\n",
				*(long *)(2+((symtab+i)->st_value)+16+(long)*(long *)(((int)((symtab+i)->st_value))+12))
			);

#endif
		}
	}
	printf("Total: %d symbols; plt_min=0x%08lx, plt_max=0x%08lx\n", nsymbols, plt_min, plt_max);

	buffer[0] = 0;
	buffer[1] = nsymbols * sizeof(struct trampoline_t) + 2 * nsymbols * sizeof(void *);
	buffer[2] = PROT_READ | PROT_WRITE | PROT_EXEC;
	buffer[3] = MAP_PRIVATE | MAP_ANON;
	buffer[4] = 0;
	buffer[5] = 0;
	GOT = (void **)_sys_mmap(buffer);
	if (!GOT) {
		printf("ltrace: Cannot mmap?\n");
		return;
	}
	names = (char **)(GOT + nsymbols);
	table = (struct trampoline_t *)(names + nsymbols);
	plt_min = plt_min & ~(4096-1);
	printf("plt_min=0x%08x, GOT=0x%08x, names=0x%08x, table=0x%08x\n", plt_min,GOT,names,table);
	i = _sys_mprotect((void*)plt_min, plt_max-plt_min+6, PROT_READ | PROT_WRITE | PROT_EXEC);
	if (i<0) {
		printf("ltrace: Cannot mprotect?\n");
		return;
	}
	j=0;
	for(i=0; i<symtab_len/sizeof(struct elf32_sym); i++) {
		if (!((symtab+i)->st_shndx) && (symtab+i)->st_value) {
			void * table_tmp;
			GOT[j] = (void **)*(long *)(((int)((symtab+i)->st_value))+2);
			names[j] = (strtab+(symtab+i)->st_name);
			bcopy(trampoline, (char *)&table[j], sizeof(struct trampoline_t));
			table[j].aqui = (void *)(&table[j]) + sizeof(void *);
			table[j].function_no = j;
			table[j].new_func = new_func_ptr;
			printf("GOT[%d] = 0x%08x\n", j, GOT[j]);
			printf("names[%d] = \"%s\"\n", j, names[j]);
			printf("table[%d].aqui = 0x%08x\n", j, table[j].aqui);
			printf("table[%d].function_no = 0x%08x\n", j, table[j].function_no);
			printf("table[%d].new_func = 0x%08x\n", j, table[j].new_func);
			table_tmp = &table[j];
#if 0 /* HCK: aun no funciona */
			bcopy(table_tmp, (void*)(((int)((symtab+i)->st_value))+2), sizeof(void*));
#endif
			j++;
		}
	}
#if 1
	return;
}
static void new_func(void)
{
	_sys_sync();
	printf("Se ha llamado a una funcion!\n");
	_sys__exit(2);
}
#else

	j=0;
	for(i=0; i<symtab_len/sizeof(struct elf32_sym); i++) {
		if (!((symtab+i)->st_shndx) && (symtab+i)->st_value) {
#if 0
			bcopy(ax_jmp, (char *)&table[j], sizeof(struct ax_jmp_t));
			table[j].src = (char *)&table[j];
			table[j].dst = &pointer_tmp;
			table[j].new_func = &new_func_ptr;
			table[j].name = strtab+(symtab+i)->st_name;
			table[j].got = (u_long *)*(long *)(((int)((symtab+i)->st_value))+2);
			table[j].real_func = (void *)*(table[j].got);
			k = &table[j];
			bcopy((char*)&k, (char *)table[j].got, 4);
			j++;
#endif
		}
	}

	_sys_write(fd,"ltrace starting...\n",19);
}

static u_long where_to_return;
static u_long returned_value;
static u_long where_to_jump;

static int reentrant=0;

static void print_results(u_long arg);

static void kk(void)
{
	char buf[1024];
	sprintf(buf, "\n%s(", pointer_tmp->name);
	_sys_write(fd,buf,strlen(buf));
}

static void new_func(void)
{
#if 1
	kk();
#endif
	if (reentrant) {
#if 0
_sys_write(fd,"reentrant\n",10);
#endif
		__asm__ __volatile__(
			"movl %%ebp, %%esp\n\t"
			"popl %%ebp\n\t"
			"jmp *%%eax\n"
			: : "a" (pointer_tmp->real_func)
		);
	}
	reentrant=1;
	pointer = pointer_tmp;
	where_to_jump = (u_long)pointer->real_func;

	/* This is only to avoid a GCC warning; shouldn't be here:
	 */
	where_to_return = (long)print_results;

	/* HCK: Is all these stuff about 'ebp' buggy? */
	__asm__ __volatile__(
		"movl %ebp, %esp\n\t"
		"popl %ebp\n\t"
		"popl %eax\n\t"
		"movl %eax, where_to_return\n\t"
		"movl where_to_jump, %eax\n\t"
		"call *%eax\n\t"
		"movl %eax, returned_value\n\t"
		"call print_results\n\t"
		"movl where_to_return, %eax\n\t"
		"pushl %eax\n\t"
		"movl returned_value, %eax\n\t"
		"movl $0, reentrant\n\t"
		"ret\n"
	);
}

#endif
