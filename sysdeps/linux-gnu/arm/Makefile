OBJ	=	trace.o regs.o

all:		arch.o

arch.o:		$(OBJ) arch.h
		$(LD) -r -o arch.o $(OBJ)

clean:
		$(RM) $(OBJ) arch.o

