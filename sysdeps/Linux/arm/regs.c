#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

#define off_pc 60
#define off_lr 56
#define off_sp 52

int get_instruction_pointer(pid_t pid)
{
	return ptrace(PTRACE_PEEKUSER, pid, off_pc, 0);
}

int get_stack_pointer(pid_t pid)
{
	return ptrace(PTRACE_PEEKUSER, pid, off_sp, 0);
}

/* really, this is given the *stack_pointer expecting
 * a CISC architecture; in our case, we don't need that */
int get_return_addr(pid_t pid, void * stack_pointer)
{
	return ptrace(PTRACE_PEEKUSER, pid, off_lr, 0);
}

