CC	=	gcc
CFLAGS	=	-Wall -Werror -O2

all:	ltrace

clean:
	rm -f ltrace ltrace.o

dist:
	make clean
	( cd .. ; tar zcvf ltrace2-`date +%y%m%d`.tgz ltrace2 )
