OBJ	=	regs.o trace.o

all:		arch.o

arch.o:		$(OBJ)
		$(LD) -r -o arch.o $(OBJ)

clean:
		$(RM) $(OBJ) arch.o
