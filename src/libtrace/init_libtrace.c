#include <fcntl.h>
#include <sys/types.h>
#include <linux/elf.h>
#include <hck/syscall.h>
#include <sys/mman.h>

static int fd = 2;

static char ax_jmp[] = {
	0xb8, 1, 2, 3, 4,		/* movl $0x04030201, %eax	*/
	0xa3, 5, 6, 7, 8,		/* movl %eax, 0x08070605	*/
	0xff, 0x25, 1, 2, 3, 4,		/* jmp *0x04030201		*/
	0, 0, 0, 0,			/* real_func			*/
	0, 0, 0, 0,			/* name				*/
	0, 0, 0, 0			/* got				*/
};

struct ax_jmp_t {
	char tmp1;
	void * src __attribute__ ((packed));
	char tmp2;
	void * dst __attribute__ ((packed));
	char tmp3,tmp4;
	void * new_func __attribute__ ((packed));
	void * real_func __attribute__ ((packed));
	char * name __attribute__ ((packed));
	u_long * got __attribute__ ((packed));
};

static struct ax_jmp_t * pointer_tmp;
static struct ax_jmp_t * pointer;

static void new_func(void);
static void * new_func_ptr = NULL;

static int current_pid;

static void init_libtrace(void)
{
	int i,j;
	void * k;
	struct elf32_sym * symtab = NULL;
	size_t symtab_len = 0;
	char * strtab = NULL;
	char * tmp;
	int nsymbols = 0;
	long buffer[6];
	struct ax_jmp_t * table;

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
		if (fd<0) {
			_sys_write(2, "Not enough fd's?\n", 17);
			return;
		}
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

	for(i=0; i<symtab_len/sizeof(struct elf32_sym); i++) {
		if (!((symtab+i)->st_shndx) && (symtab+i)->st_value) {
			nsymbols++;
#if 0
			_sys_write(fd, "New symbol: ", 12);
			_sys_write(fd,strtab+(symtab+i)->st_name,strlen(strtab+(symtab+i)->st_name));
			_sys_write(fd, "\n", 1);
#endif
		}
	}

	buffer[0] = 0;
	buffer[1] = nsymbols * sizeof(struct ax_jmp_t);
	buffer[2] = PROT_READ | PROT_WRITE | PROT_EXEC;
	buffer[3] = MAP_PRIVATE | MAP_ANON;
	buffer[4] = 0;
	buffer[5] = 0;
	table = (struct ax_jmp_t *)_sys_mmap(buffer);
	if (!table) {
		_sys_write(fd,"Cannot mmap?\n",13);
		return;
	}

	j=0;
	for(i=0; i<symtab_len/sizeof(struct elf32_sym); i++) {
		if (!((symtab+i)->st_shndx) && (symtab+i)->st_value) {
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
		}
	}

	_sys_write(fd,"ltrace starting...\n",19);
}

static u_long where_to_return;
static u_long returned_value;
static u_long where_to_jump;

static int reentrant=0;

static void print_results(u_long arg);

static void new_func(void)
{
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
	where_to_return = returned_value = (u_long)print_results;

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
