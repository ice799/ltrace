OBJ	=	trace.o regs.o ffcheck.o

all:		arch.o

arch.o:		$(OBJ)
		$(LD) -r -o arch.o $(OBJ)

clean:
		$(RM) $(OBJ) arch.o
