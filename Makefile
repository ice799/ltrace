CC	=	gcc
CFLAGS	=	-Wall -O2 -Iinclude
LD	=	ld

all:
	@echo 'Type "make dist" to make a distribution .tgz'
	@echo

clean:
	make -C src/libtrace clean
	make -C src/ltrace clean
	make -C src/test clean
	rm -f *.o

dist:
	make clean
	( cd .. ; tar zcvf ltrace-`date +%y%m%d`.tgz ltrace )
