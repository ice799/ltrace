CC	=	gcc
CFLAGS	=	-Wall -O2 -I../include
LD	=	ld

all:		show-elf libtest.so

show-elf:	show-elf.o hacks.o

libtest.so:	libtest.o
		ld -shared -m elf_i386 -soname libtest.so -o libtest.so libtest.o

libtest.o:	libtest.s

clean:
	rm -f show-elf libtest.so *.o
