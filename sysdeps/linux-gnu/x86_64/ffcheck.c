#include <elf.h>

int
ffcheck (void *maddr)
{
	Elf64_Ehdr	*ehdr=maddr;

	if (! ehdr)
		return 0;
	if (
			ehdr->e_type == 2 &&
			ehdr->e_machine == 0x3e &&
			ehdr->e_version == 1
	   )
		return 1;

	return 0;
}
