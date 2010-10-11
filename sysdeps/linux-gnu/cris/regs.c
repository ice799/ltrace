#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "common.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void *get_instruction_pointer(Process *proc)
{
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_PPC, 0);
}

void set_instruction_pointer(Process *proc, void *addr)
{
	ptrace(PTRACE_POKEUSER, proc->pid, 4 * PT_PPC, addr);
}

void *get_stack_pointer(Process *proc)
{
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_USP, 0);
}

void *get_return_addr(Process *proc, void *stack_pointer)
{
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, 4 * PT_SRP, 0);
}
