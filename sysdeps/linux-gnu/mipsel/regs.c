#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stddef.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <linux/user.h>

#include "common.h"
#include "mipsel.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

/**
   \addtogroup mipsel
   @{
 */


/**
   \param proc The process to work on.
   \return The current instruction pointer.
 */
void *
get_instruction_pointer(Process *proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, off_pc, 0);
}

/**
   \param proc The process to work on.
   \param addr The address to set to.

   Called by \c continue_after_breakpoint().

   \todo Our mips kernel ptrace doesn't support PTRACE_SINGLESTEP, so
   we \c continue_process() after a breakpoint. Check if this is OK.
 */
void
set_instruction_pointer(Process *proc, void *addr) {
	ptrace(PTRACE_POKEUSER, proc->pid, off_pc, addr);
}

/**
   \param proc The process to work on.
   \return The current stack pointer.
 */
void *
get_stack_pointer(Process *proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, off_sp, 0);
}

/**
   \param proc The process to work on.
   \param stack_pointer The current stack pointer for proc
   \return The current return address.

   Called by \c handle_breakpoint().

   Mips uses r31 for the return address, so the stack_pointer is
   unused.
 */
void *
get_return_addr(Process *proc, void *stack_pointer) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, off_lr, 0);
}

void
set_return_addr(Process *proc, void *addr) {
	ptrace(PTRACE_POKEUSER, proc->pid, off_lr, addr);
}
