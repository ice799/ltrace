#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/elf.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>

static int debug = 0;

struct library_symbol {
	char * name;
	unsigned long addr;
	unsigned char value;
	struct library_symbol * next;
};

struct library_symbol * library_symbols = NULL;

int read_elf(char *filename)
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
#if 0
				strtab = (char *)(addr + shdr->sh_offset);
#else
				strtab = (char *)(addr + shdr->sh_offset);
#endif
			}
		}
	}
	if (debug>0) {
		fprintf(stderr, "symtab: 0x%08x\n", (unsigned)symtab);
		fprintf(stderr, "symtab_len: %lu\n", symtab_len);
		fprintf(stderr, "strtab: 0x%08x\n", (unsigned)strtab);
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
			library_symbols->addr = ((symtab+i)->st_value);
			library_symbols->name = strtab+(symtab+i)->st_name;
			library_symbols->next = tmp;
			if (debug>0) {
				fprintf(stderr, "addr: 0x%08x, symbol: \"%s\"\n",
					(unsigned)((symtab+i)->st_value),
					(strtab+(symtab+i)->st_name));
			}
		}
	}
	return 1;
}

int main(int argc, char **argv)
{
	int pid;
	int status;
	struct library_symbol * tmp = NULL;

	while ((argc>1) && (argv[1][0] == '-') && (argv[1][2] == '\0')) {
		switch(argv[1][1]) {
			case 'd':	debug++;
					break;
			default:	fprintf(stderr, "Unknown option '%c'\n", argv[1][1]);
					exit(1);
		}
		argc--; argv++;
	}

	if (argc<2) {
		fprintf(stderr, "Usage: %s [<options>] <program> [<arguments>]\n", argv[0]);
		exit(1);
	}
	if (!read_elf(argv[1])) {
		fprintf(stderr, "%s: Not dynamically linked\n", argv[0]);
		exit(1);
	}
	pid = fork();
	if (pid<0) {
		perror("fork");
		exit(1);
	} else if (!pid) {
		if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
			perror("PTRACE_TRACEME");
			exit(1);
		}
		execvp(argv[1], argv+1);
		fprintf(stderr, "Can't execute \"%s\": %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}
	fprintf(stderr, "pid %u attached\n", pid);

	/* Enable breakpoints: */
	pid = wait4(-1, &status, 0, NULL);
	if (pid==-1) {
		perror("wait4");
		exit(1);
	}
	fprintf(stderr, "Enabling breakpoints...\n");
	tmp = library_symbols;
	while(tmp) {
		int a;
		a = ptrace(PTRACE_PEEKTEXT, pid, tmp->addr, 0);
		tmp->value = a & 0xFF;
		a &= 0xFFFFFF00;
		a |= 0xCC;
		ptrace(PTRACE_POKETEXT, pid, tmp->addr, a);
		tmp = tmp->next;
	}
	ptrace(PTRACE_CONT, pid, 1, 0);

	while(1) {
		int eip;
		int function_seen;

		pid = wait4(-1, &status, 0, NULL);
		if (pid==-1) {
			if (errno == ECHILD) {
				fprintf(stderr, "No more children\n");
				exit(0);
			}
			perror("wait4");
			exit(1);
		}
		if (WIFEXITED(status)) {
			fprintf(stderr, "pid %u exited\n", pid);
			continue;
		}
		if (WIFSIGNALED(status)) {
			fprintf(stderr, "pid %u exited on signal %u\n", pid, WTERMSIG(status));
			continue;
		}
		if (!WIFSTOPPED(status)) {
			fprintf(stderr, "pid %u ???\n", pid);
			continue;
		}
		if (WSTOPSIG(status) != SIGTRAP) {
			fprintf(stderr, "Signal: %u\n", WSTOPSIG(status));
			ptrace(PTRACE_CONT, pid, 1, WSTOPSIG(status));
			continue;
		}
		/* pid is stopped... */
		eip = ptrace(PTRACE_PEEKUSR, pid, 4*EIP, 0);
/*
		fprintf(stderr,"EIP = 0x%08x\n", eip);
		fprintf(stderr,"EBP = 0x%08x\n", ptrace(PTRACE_PEEKUSR, pid, 4*EBP, 0));
*/
		tmp = library_symbols;
		function_seen = 0;
		while(tmp) {
			if (eip == tmp->addr+1) {
				int a;
				function_seen = 1;
				fprintf(stderr, "Function: %s\n", tmp->name);
				a = ptrace(PTRACE_PEEKTEXT, pid, tmp->addr, 0);
				a &= 0xFFFFFF00;
				a |= tmp->value;
				ptrace(PTRACE_POKETEXT, pid, tmp->addr, a);
				ptrace(PTRACE_POKEUSR, pid, 4*EIP, eip-1);
				ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
				pid = wait4(-1, &status, 0, NULL);
				if (pid==-1) {
					if (errno == ECHILD) {
						fprintf(stderr, "No more children\n");
						exit(0);
					}
					perror("wait4");
					exit(1);
				}
				a = ptrace(PTRACE_PEEKTEXT, pid, tmp->addr, 0);
				a &= 0xFFFFFF00;
				a |= 0xCC;
				ptrace(PTRACE_POKETEXT, pid, tmp->addr, a);
				ptrace(PTRACE_CONT, pid, 1, 0);
				break;
			}
			tmp = tmp->next;
		}
		if (!function_seen) {
			fprintf(stderr, "pid %u stopped; continuing it...\n", pid);
			ptrace(PTRACE_CONT, pid, 1, 0);
		}
	}
	exit(0);
}
