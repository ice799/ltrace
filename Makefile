CC	=	gcc
CFLAGS	=	-O2 -g -Wall

OBJ	=	ltrace.o functions.o elf.o i386.o symbols.o process.o

all:		build

build:		ltrace

ltrace:		ltrace.o $(OBJ)

clean:
		rm -f ltrace $(OBJ)

dist:		clean
		( cd .. ; tar zcvf ltrace-`date +%y%m%d`.tgz ltrace )

install:	build
		install -d $(DESTDIR)/usr/bin $(DESTDIR)/usr/doc/ltrace $(DESTDIR)/etc $(DESTDIR)/usr/man/man1
		install -s ltrace $(DESTDIR)/usr/bin
		install -m 644 README $(DESTDIR)/usr/doc/ltrace
		install -m 644 ltrace.1 $(DESTDIR)/usr/man/man1
