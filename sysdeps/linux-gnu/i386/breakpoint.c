#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include <assert.h>
#include "ltrace.h"
#include "options.h"
#include "output.h"

void
enable_breakpoint(pid_t pid, struct breakpoint * sbp) {
	int a;

	if (opt_d>1) {
		output_line(0, "enable_breakpoint(%d,0x%08x)", pid, sbp->addr);
	}
	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	assert((a & 0xFF) != 0xCC);
	sbp->orig_value[0] = a & 0xFF;
	a &= 0xFFFFFF00;
	a |= 0xCC;
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}

void
disable_breakpoint(pid_t pid, const struct breakpoint * sbp) {
	int a;

	if (opt_d>1) {
		output_line(0, "disable_breakpoint(%d,0x%08x)", pid, sbp->addr);
	}
	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);
	assert((a & 0xFF) == 0xCC);
	a &= 0xFFFFFF00;
	a |= sbp->orig_value[0];
	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}
