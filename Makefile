CC	=	gcc
CFLAGS	=	-g -Wall -O2

OBJ	=	ltrace.o functions.o elf.o i386.o trace.o

all:		build

build:		ltrace

ltrace:		ltrace.o $(OBJ)

clean:
		rm -f ltrace $(OBJ)

dist:		clean
		( cd .. ; tar zcvf ltrace-`date +%y%m%d`.tgz ltrace )

install:	build
		install -d $(DESTDIR)/usr/bin $(DESTDIR)/usr/doc/ltrace
		install -s ltrace $(DESTDIR)/usr/bin
		install -m 644 README debian/copyright debian/changelog $(DESTDIR)/usr/doc/ltrace
