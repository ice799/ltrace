#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include "ltrace.h"

void insert_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	unsigned long a;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	*(unsigned long *)sbp->orig_value = a;
	a = ((0x91 * 256 + 0xd0) * 256 + 0x20) * 256 + 0x01;
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}

void delete_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, *(long *)sbp->orig_value);
}
