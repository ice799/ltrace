OBJ	=	trace.o regs.o plt.o breakpoint.o

all:		arch.o

arch.o:		$(OBJ) arch.h
		$(CC) -nostdlib -r -o arch.o $(OBJ)

clean:
		$(RM) $(OBJ) arch.o

