TOPDIR	=	/home/cespedes/c/ltrace
CC	=	gcc
CFLAGS	=	-Wall -O2 -I$(TOPDIR)/include
LD	=	ld

OBJS	=	__setfpucw.o init_libtrace.o

all:		libtrace.so.1

libtrace.so.1:	$(OBJS)
		$(LD) -shared -m elf_i386 -soname $@ -o $@ $(OBJS)
		cp libtrace.so.1 $(TOPDIR)/lib

clean:
		rm -f $(OBJS) libtrace.so.1
