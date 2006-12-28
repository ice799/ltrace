.PHONY: all deps clean docs

OBJ	=	trace.o regs.o plt.o
SRC=$(OBJ:.o=.c)

all:		arch.o

deps:
	$(CC) $(CFLAGS) $(CPPFLAGS) -M  $(SRC) > .depends

arch.o:		$(OBJ) arch.h
		$(CC) -nostdlib -r -o arch.o $(OBJ)

clean:
	-rm -f $(OBJ) arch.o
	-rm -rf html

docs:
	doxygen


-include .depends
