##ARCH	:=	$(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/)
OS	:=	$(shell uname -s)

TOPDIR	=	$(shell pwd)

CC	=	gcc
CFLAGS	=	-O2 -g -Wall -I$(TOPDIR) -I$(TOPDIR)/sysdeps/$(OS) -I- #-I$(TOPDIR)/sysdeps/$(ARCH)

OBJ	=	ltrace.o options.o elf.o output.o config_file.o	\
		execute_program.o wait_for_something.o process_event.o \
		display_args.o breakpoints.o

all:		dummy
		$(MAKE) -C sysdeps/$(OS)
		$(MAKE) ltrace

ltrace:		sysdeps/sysdep.o $(OBJ)

clean:
		$(MAKE) -C sysdeps/$(OS) clean
		rm -f ltrace $(OBJ)

dist:		clean
		( cd .. ; tar zcvf ltrace-`date +%y%m%d`.tgz ltrace )

install:	ltrace
		install -d $(DESTDIR)/usr/bin $(DESTDIR)/usr/doc/ltrace $(DESTDIR)/usr/man/man1
		install -d $(DESTDIR)/etc
		install -s ltrace $(DESTDIR)/usr/bin
		install -m 644 etc/ltrace.conf $(DESTDIR)/etc
		install -m 644 COPYING README TODO BUGS $(DESTDIR)/usr/doc/ltrace
		install -m 644 ltrace.1 $(DESTDIR)/usr/man/man1

dummy:

.EXPORT_ALL_VARIABLES:
