#include <sys/types.h>
#include <sys/ptrace.h>

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

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

