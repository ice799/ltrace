#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include "ltrace.h"

void enable_breakpoint(pid_t pid, struct breakpoint * sbp)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	sbp->orig_value[0] = (a & 0xFF000000) >> 24;
	sbp->orig_value[1] = (a & 0x00FF0000) >> 16;
	a &= 0x0000FFFF;
	a |= 0x4E4F0000;
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}

void disable_breakpoint(pid_t pid, const struct breakpoint * sbp)
{
	int a;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	a &= 0x0000FFFF;
	a |= (sbp->orig_value[0] << 24) | (sbp->orig_value[1] << 16);
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}
