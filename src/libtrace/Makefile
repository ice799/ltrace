TOPDIR	=	/home/cespedes/c/ltrace
CC	=	gcc
CFLAGS	=	-Wall -O2 -I$(TOPDIR)/include
LD	=	ld

OBJS	=	libtrace.o

all:		libtrace.so.1

libtrace.o:	libtrace.c init_libtrace.c strlen.c strncmp.c getenv.c atoi.c bcopy.c print_results.c sprintf.c vsprintf.c strcmp.c strnlen.c strcat.c

libtrace.so.1:	$(OBJS)
		$(LD) -shared -m elf_i386 -soname $@ -o $@ $(OBJS)
		cp libtrace.so.1 $(TOPDIR)/lib

clean:
		rm -f $(OBJS) libtrace.so.1
