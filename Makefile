CC	=	gcc
CFLAGS	=	-Wall -O2 -Iinclude
LD	=	ld

all:
	@echo 'Type "make dist" to make a distribution .tgz'
	@echo

clean:
	make -C src clean
	make -C test clean
	rm -f *.o

dist:
	make clean
	( cd .. ; tar zcvf libtrace-`date +%y%m%d`.tgz libtrace )
