#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ptrace.h>

void * get_instruction_pointer(pid_t pid)
{
	return (void *)ptrace(PTRACE_PEEKUSER, pid, PT_PC, 0);
}

void * get_stack_pointer(pid_t pid)
{
	return (void *)-1;
}

void * get_return_addr(pid_t pid, void * stack_pointer)
{
	return (void *)-1;
}

