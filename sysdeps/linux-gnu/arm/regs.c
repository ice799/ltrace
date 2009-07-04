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

#define off_pc 60
#define off_lr 56
#define off_sp 52

void *
get_instruction_pointer(Process *proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, off_pc, 0);
}

void
set_instruction_pointer(Process *proc, void *addr) {
	ptrace(PTRACE_POKEUSER, proc->pid, off_pc, addr);
}

void *
get_stack_pointer(Process *proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, off_sp, 0);
}

/* really, this is given the *stack_pointer expecting
 * a CISC architecture; in our case, we don't need that */
void *
get_return_addr(Process *proc, void *stack_pointer) {
	long addr = ptrace(PTRACE_PEEKUSER, proc->pid, off_lr, 0);

	proc->thumb_mode = addr & 1;
	if (proc->thumb_mode)
		addr &= ~1;
	return (void *)addr;
}

void
set_return_addr(Process *proc, void *addr) {
	ptrace(PTRACE_POKEUSER, proc->pid, off_lr, addr);
}
