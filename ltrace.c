#include <stdio.h>
#include <errno.h>
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

u_long strtab;
u_long symtab;
u_long symtab_len;

int read_elf(char *filename)
{
	struct stat sbuf;
	int fd;
	void * addr;
	struct elf32_hdr * hdr;
	Elf32_Shdr * shdr;
	int i;

	strtab = symtab = symtab_len = 0;

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
				symtab = shdr->sh_addr;
				symtab_len = shdr->sh_size;
			}
		}
		if (shdr->sh_type == SHT_STRTAB) {
			if (!strtab) {
				strtab = shdr->sh_addr;
			}
		}
	}
	fprintf(stderr, "symtab: 0x%08lx\n", symtab);
	fprintf(stderr, "symtab_len: %lu\n", symtab_len);
	fprintf(stderr, "strtab: 0x%08lx\n", strtab);
	return 0;
}

int main(int argc, char **argv)
{
	int pid;
	int status;
	struct rusage ru;

	if (argc<2) {
		fprintf(stderr, "Usage: %s <program> [<arguments>]\n", argv[0]);
		exit(1);
	}
	read_elf(argv[1]);
	if (!symtab) {
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
	while(1) {
		pid = wait4(-1, &status, 0, &ru);
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
		fprintf(stderr,"EIP = 0x%08x\n", ptrace(PTRACE_PEEKUSR, pid, 4*EIP, 0));
		fprintf(stderr,"EBP = 0x%08x\n", ptrace(PTRACE_PEEKUSR, pid, 4*EBP, 0));
#if 0
		ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
		continue;
#endif
		if (WIFSTOPPED(status)) {
			fprintf(stderr, "pid %u stopped; continuing it...\n", pid);
			ptrace(PTRACE_CONT, pid, 1, 0);
                } else {
			fprintf(stderr, "pid %u ???\n", pid);
		}
	}
	exit(0);
}
