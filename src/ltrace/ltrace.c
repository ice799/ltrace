#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc<2) {
		fprintf(stderr, "Usage: %s <program> [<arguments>]\n", argv[0]);
		exit(1);
	}
	setenv("LD_PRELOAD", TOPDIR "/lib/libtrace.so.1", 1);
	execve(argv[1], &argv[1], environ);
	fprintf(stderr, "Couldn't execute %s\n", argv[1]);
	exit(1);
}
