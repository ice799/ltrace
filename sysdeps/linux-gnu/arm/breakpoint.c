#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include "ltrace.h"

void insert_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	sbp->orig_value[0] = a;
	sbp->orig_value[1] = a>>8;
	sbp->orig_value[2] = a>>16;
	sbp->orig_value[3] = a>>24;
	a = BREAKPOINT_VALUE;
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}

void delete_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	int a;

	a = sbp->orig_value[0] + (sbp->orig_value[1]<<8) + (sbp->orig_value[2]<<16) + (sbp->orig_value[3]<<24);
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}
