#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include "ltrace.h"

void insert_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	sbp->orig_value[0] = a & 0xFF;
	a &= 0xFFFFFF00;
	a |= 0xCC;
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}

void delete_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	a &= 0xFFFFFF00;
	a |= sbp->orig_value[0];
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}
