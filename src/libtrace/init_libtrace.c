#include <linux/elf.h>
#include <hck/syscall.h>

static int errno;

void init_libtrace(void)
{
	unsigned long tmp[0];
	unsigned long *stack = tmp;
	int phnum=0, phent=0;
	struct elf_phdr * phdr = NULL;
	int i;
	struct dynamic * dpnt;
	struct elf32_sym * symtab = NULL;
	void * strtab = NULL;

/*
 * When loading the interpreter, in linux/fs/binfmt_elf.h, the stack
 * is filled with an auxiliary table defined in include/linux/elf.h;
 * it has some important things such as pointers to the program
 * headers.  We are trying to find that table.
 *
 */
	for(; (stack < (unsigned long *)(0xc0000000-256)); stack++) {
		if ((stack[0]==3) && (stack[2]==4) && (stack[4]==5) && (stack[6]==6)) {
			phdr = (struct elf_phdr *)stack[1];
			phent = stack[3];
			phnum = stack[5];
			break;
		}
	}
	if (!phdr) {
		_sys_write(2,"No auxiliary table in stack?\n",29);
		return;
	}

	for(i=0; i<phnum; i++) {
		if (phdr->p_type == PT_DYNAMIC) {
			break;
		}
		phdr = (struct elf_phdr *)((void*)phdr + phent);
	}
	if (!phdr || phdr->p_type != PT_DYNAMIC) {
		_sys_write(2,"No .dynamic in file?\n",21);
		return;
	}
	dpnt = (struct dynamic *)phdr->p_vaddr;

	while(dpnt->d_tag) {
		if (dpnt->d_tag == DT_SYMTAB) {
			symtab = (struct elf32_sym *) dpnt->d_un.d_ptr;
		}
		if (dpnt->d_tag == DT_STRTAB) {
			strtab = (void *) dpnt->d_un.d_ptr;
		}
		dpnt++;
	}
	if (!symtab) {
		_sys_write(2, "No .symtab?\n",12);
		return;
	}
	if (!strtab) {
		_sys_write(2, "No .strtab?\n",12);
		return;
	}

	_sys_write(2,"Hola\n",5);
}
