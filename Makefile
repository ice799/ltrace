CC	=	gcc
CFLAGS	=	-Wall -O2

all:	ltrace

clean:
	rm -f ltrace ltrace.o

dist:	#clean
	( cd .. ; tar zcvf ltrace2-`date +%y%m%d`.tgz ltrace2 )
