#include <sys/types.h>
#include <sys/ptrace.h>

int get_instruction_pointer(pid_t pid)
{
	return ptrace(PTRACE_PEEKUSER, pid, 4*EIP, 0);
}

int get_stack_pointer(pid_t pid)
{
	return ptrace(PTRACE_PEEKUSER, pid, 4*UESP, 0);
}

int get_return_addr(pid_t pid, void * stack_pointer)
{
	return ptrace(PTRACE_PEEKTEXT, pid, stack_pointer, 0);
}

