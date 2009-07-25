#include "config.h"

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

void *
get_instruction_pointer(Process *proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, 4 * EIP, 0);
}

void
set_instruction_pointer(Process *proc, void *addr) {
	ptrace(PTRACE_POKEUSER, proc->pid, 4 * EIP, (long)addr);
}

void *
get_stack_pointer(Process *proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, 4 * UESP, 0);
}

void *
get_return_addr(Process *proc, void *stack_pointer) {
	return (void *)ptrace(PTRACE_PEEKTEXT, proc->pid, stack_pointer, 0);
}

void
set_return_addr(Process *proc, void *addr) {
	ptrace(PTRACE_POKETEXT, proc->pid, proc->stack_pointer, (long)addr);
}
