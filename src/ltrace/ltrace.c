#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/elf.h>
#include <string.h>

int main(int argc, char **argv)
{
	int fd;
	struct stat sbuf;
	size_t filesize;
	void * addr;
	struct elf32_hdr * hdr;
	Elf32_Shdr * shdr;
	int i;
	u_long strtab = 0;
	u_long symtab = 0;
	u_long symtab_len = 0;
	char buf[1024];
	char * debug_filename = NULL;

	if (argc<2) {
		fprintf(stderr, "Usage: %s [options] <program> [<arguments>]\n", argv[0]);
		exit(1);
	}

	while ((argv[1][0] == '-') && argv[1][1] && !argv[1][2]) {
		switch(argv[1][1]) {
			case 'o':
				debug_filename = argv[2];
				argc--; argv++;
				break;
			default:
				fprintf(stderr, "Unknown option: '%c'\n", argv[1][1]);
		}
		argc--; argv++;
	}

	if (argc<2) {
		fprintf(stderr, "Usage: %s [options] <program> [<arguments>]\n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);

	if (fd<0) {
		fprintf(stderr, "Can't open \"%s\" for reading: %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}

	if (fstat(fd, &sbuf)<0) {
		fprintf(stderr, "Can't stat \"%s\": %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}
	filesize = sbuf.st_size;
	if (filesize < sizeof(struct elf32_hdr)) {
		fprintf(stderr, "\"%s\" is not an ELF object\n", argv[1]);
		exit(1);
	}

	addr = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
	if (!addr) {
		fprintf(stderr, "Can't mmap \"%s\": %s\n", argv[1], sys_errlist[errno]);
		exit(1);
	}

	if (debug_filename) {
		setenv("LTRACE_FILENAME", debug_filename, 1);
		printf("LTRACE_FILENAME=%s\n", debug_filename);
	}

/*
 * Tengo que decirle a libtrace:
 *  - Comienzo y tamanno de '.dynsym'	LTRACE_SYMTAB_ADDR,	LTRACE_SYMTAB_SIZE
 *  - Comienzo de '.dynstr'		LTRACE_STRTAB_ADDR
 *  - Comienzo y tamanno de '.got'	LTRACE_GOT_ADDR,	LTRACE_GOT_SIZE
 *
 * Todo ello especificado en decimal (por elegir algo)
 */

	hdr = addr;

	if (strncmp(hdr->e_ident, ELFMAG, SELFMAG)) {
		fprintf(stderr, "%s is not an ELF object\n", argv[1]);
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
	if (!symtab) {
		fprintf(stderr, "No .dynsym in file. Not dynamically linked?\n");
		exit(1);
	}
	if (!strtab) {
		fprintf(stderr, "No .dynstr in file. Not dynamically linked?\n");
		exit(1);
	}

	sprintf(buf,"%lu", symtab);
	printf("LTRACE_SYMTAB=%lu\n", symtab);
	setenv("LTRACE_SYMTAB", buf, 1);
	sprintf(buf,"%lu", symtab_len);
	printf("LTRACE_SYMTAB_LEN=%lu\n", symtab_len);
	setenv("LTRACE_SYMTAB_LEN", buf, 1);
	sprintf(buf,"%lu", strtab);
	printf("LTRACE_STRTAB=%lu\n", strtab);
	setenv("LTRACE_STRTAB", buf, 1);

	setenv("LD_PRELOAD", TOPDIR "/lib/libtrace.so.1", 1);
	execve(argv[1], &argv[1], environ);
	fprintf(stderr, "Couldn't execute \"%s\": %s\n", argv[1], sys_errlist[errno]);
	exit(1);
}
