CC	=	gcc
CFLAGS	=	-Wall -O2 -I../include
LD	=	ld

OBJS	=	__setfpucw.o init_libtrace.o

all:		libtrace.so.1

libtrace.so.1:	$(OBJS)
		$(LD) -shared -m elf_i386 -soname $@ -o $@ $(OBJS)

clean:
		rm -f $(OBJS) libtrace.so.1
