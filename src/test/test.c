#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <linux/elf.h>

extern void * fd_to_memory(int fd, unsigned long * length);

int main(int argc, char **argv)
{
	int fd;
	unsigned long length;
	int i;
	void * addr;
	Elf32_Ehdr * hdr;
	struct elf_phdr * phdr;
	Elf32_Shdr * shdr;

	if (argc!=2) {
		fprintf(stderr, "Usage: %s <elf_executable>\n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd<0) {
		fprintf(stderr, "Cannot open %s\n", argv[1]);
		exit(1);
	}

	addr = fd_to_memory(fd, &length);
	if (!addr) {
		exit(1);
	}

	hdr = addr;

	if (strncmp(hdr->e_ident, ELFMAG, SELFMAG)) {
		fprintf(stderr, "%s is not an ELF object\n", argv[1]);
		exit(1);
	}
	printf("Filename:    %s\n", argv[1]);
	printf("* hdr:       0x%08x\n", 0x08000000);
	printf("EI_CLASS:    %d\n", hdr->e_ident[EI_CLASS]);
	printf("EI_DATA:     %d\n", hdr->e_ident[EI_DATA]);
	printf("EI_VERSION:  %d\n", hdr->e_ident[EI_VERSION]);
	printf("e_type:      %d\n", hdr->e_type);
	printf("e_machine:   %d\n", hdr->e_machine);
	printf("e_version:   %ld\n", hdr->e_version);
	printf("e_entry:     0x%08lx\n", hdr->e_entry);
	printf("e_phoff:     %ld\n", hdr->e_phoff);
	printf("e_shoff:     %ld\n", hdr->e_shoff);
	printf("e_flags:     0x%08lx\n", hdr->e_flags);
	printf("e_ehsize:    %d\n", hdr->e_ehsize);
	printf("e_phentsize: %d\n", hdr->e_phentsize);
	printf("e_phnum:     %d\n", hdr->e_phnum);
	printf("e_shentsize: %d\n", hdr->e_shentsize);
	printf("e_shnum:     %d\n", hdr->e_shnum);
	printf("e_shstrndx:  %d\n", hdr->e_shstrndx);

	for(i=0; i<hdr->e_phnum; i++) {
		printf("-- phdr number %d --\n", i);
		phdr = addr + hdr->e_phoff + i*hdr->e_phentsize;
		printf("* phdr:   0x%08x\n", (u_int)phdr - (u_int)hdr + 0x08000000);
		printf("p_type:   %ld\n", phdr->p_type);
		printf("p_offset: %ld\n", phdr->p_offset);
		printf("p_vaddr:  0x%08lx\n", phdr->p_vaddr);
		printf("p_paddr:  0x%08lx\n", phdr->p_paddr);
		printf("p_filesz: %ld\n", phdr->p_filesz);
		printf("p_memsz:  %ld\n", phdr->p_memsz);
		printf("p_flags:  %ld\n", phdr->p_flags);
		printf("p_align:  %ld\n", phdr->p_align);
	}
	for(i=0; i<hdr->e_shnum; i++) {
		printf("-- shdr number %d --\n", i);
		shdr = addr + hdr->e_shoff + i*hdr->e_shentsize;
		printf("* shdr:       0x%08x\n", (u_int)shdr);
		printf("sh_name:      %ld\n",shdr->sh_name);
		printf("sh_type:      %ld\n",shdr->sh_type);
		printf("sh_flags:     %ld\n",shdr->sh_flags);
		printf("sh_addr:      0x%08lx\n",shdr->sh_addr);
		printf("sh_offset:    %ld\n",shdr->sh_offset);
		printf("sh_size:      %ld\n",shdr->sh_size);
		printf("sh_link:      %ld\n",shdr->sh_link);
		printf("sh_addralign: %ld\n",shdr->sh_addralign);
		printf("sh_entsize:   %ld\n",shdr->sh_entsize);
	}

	exit(0);
}
