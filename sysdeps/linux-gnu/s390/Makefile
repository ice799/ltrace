#
# S/390 version
# Copyright (C) 2001 IBM Poughkeepsie, IBM Corporation
#
OBJ	=	trace.o regs.o plt.o

all:		arch.o

arch.o:		$(OBJ)
		$(CC) -nostdlib -r -o arch.o $(OBJ)

clean:
		$(RM) $(OBJ) arch.o
