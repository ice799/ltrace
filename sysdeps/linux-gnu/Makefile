ARCH		:=	$(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)

CPPFLAGS	+=	-I$(TOPDIR)/sysdeps/linux-gnu/$(ARCH)

OBJ		=	trace.o proc.o breakpoint.o

all:		sysdep.h signalent.h syscallent.h ../sysdep.o

sysdep.h:	$(ARCH)/arch.h
		cat $(ARCH)/arch.h > sysdep.h

signalent.h:
		cp $(ARCH)/signalent.h signalent.h

syscallent.h:
		cp $(ARCH)/syscallent.h syscallent.h

../sysdep.o:	os.o $(ARCH)/arch.o
		$(CC) -nostdlib -r -o ../sysdep.o os.o $(ARCH)/arch.o

os.o:		$(OBJ)
		$(CC) -nostdlib -r -o os.o $(OBJ)

$(ARCH)/arch.o:	dummy
		$(MAKE) -C $(ARCH)

clean:
		$(MAKE) -C $(ARCH) clean
		rm -f $(OBJ) sysdep.h signalent.h syscallent.h os.o sysdep.o ../sysdep.o

dummy:
