#include <sys/types.h>
#include <sys/ptrace.h>

int get_instruction_pointer(pid_t pid)
{
	return ptrace(PTRACE_PEEKUSER, pid, PT_PC, 0);
}

int get_stack_pointer(pid_t pid)
{
	return -1;
}

int get_return_addr(pid_t pid, void * stack_pointer)
{
	return -1;
}

