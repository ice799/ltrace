#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF 4*1024

void * fd_to_memory(int fd, unsigned long * length)
{
	void *ptr1;
	unsigned long size_alloc, size_read;
	unsigned long i;

	ptr1 = malloc(size_alloc = (BUF+sizeof(unsigned long)));
	if (!ptr1) {
		fprintf(stderr, "Not enough memory\n");
		return NULL;
	}
	size_read = i = read(fd, ptr1, BUF);
	while(i > 0) {
		if ((size_alloc - size_read) < BUF) {
			ptr1 = realloc(ptr1, size_alloc+=BUF);
			if (!ptr1) {
				fprintf(stderr, "Not enough memory\n");
				return NULL;
			}
		}
		i = read(fd, ptr1+size_read, BUF);
		size_read += i;
	}
	if (i>=0) {
		ptr1 = realloc(ptr1, size_read);
		*length = size_read;
		return ptr1;
	} else {
		perror("read");
		return NULL;
	}
}

#ifdef STANDALONE
void main(void)
{
	void * ptr;
	unsigned long length;

	ptr = fd_to_memory(0, &length);
	fprintf(stderr, "Length: %lu\n", length);
	write(1, ptr, length);
}
#endif

