#include <sys/ptrace.h>

void insert_breakpoint(int pid, unsigned long addr, unsigned char * value)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	value[0] = a & 0xFF;
	a &= 0xFFFFFF00;
	a |= 0xCC;
	ptrace(PTRACE_POKETEXT, pid, addr, a);
}

void delete_breakpoint(int pid, unsigned long addr, unsigned char * value)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	a &= 0xFFFFFF00;
	a |= value[0];
	ptrace(PTRACE_POKETEXT, pid, addr, a);
}
