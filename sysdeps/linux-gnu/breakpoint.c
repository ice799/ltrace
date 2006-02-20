#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include <string.h>
#include "arch.h"
#include "options.h"
#include "output.h"
#include "debug.h"

static unsigned char break_insn[] = BREAKPOINT_VALUE;

#ifdef ARCH_HAVE_ENABLE_BREAKPOINT
extern void arch_enable_breakpoint(pid_t, struct breakpoint *);
void enable_breakpoint(pid_t pid, struct breakpoint *sbp)
{
	arch_enable_breakpoint(pid, sbp);
}
#else
void enable_breakpoint(pid_t pid, struct breakpoint *sbp)
{
	int i, j;

	debug(1, "enable_breakpoint(%d,%p)", pid, sbp->addr);

	for (i = 0; i < 1 + ((BREAKPOINT_LENGTH - 1) / sizeof(long)); i++) {
		long a =
		    ptrace(PTRACE_PEEKTEXT, pid, sbp->addr + i * sizeof(long),
			   0);
		for (j = 0;
		     j < sizeof(long)
		     && i * sizeof(long) + j < BREAKPOINT_LENGTH; j++) {
			unsigned char *bytes = (unsigned char *)&a;

			sbp->orig_value[i * sizeof(long) + j] = bytes[+j];
			bytes[j] = break_insn[i * sizeof(long) + j];
		}
		ptrace(PTRACE_POKETEXT, pid, sbp->addr + i * sizeof(long), a);
	}
}
#endif				/* ARCH_HAVE_ENABLE_BREAKPOINT */

#ifdef ARCH_HAVE_DISABLE_BREAKPOINT
extern void arch_disable_breakpoint(pid_t, const struct breakpoint *sbp);
void disable_breakpoint(pid_t pid, const struct breakpoint *sbp)
{
	arch_disable_breakpoint(pid, sbp);
}
#else
void disable_breakpoint(pid_t pid, const struct breakpoint *sbp)
{
	int i, j;

	if (opt_d > 1) {
		output_line(0, "disable_breakpoint(%d,%p)", pid, sbp->addr);
	}

	for (i = 0; i < 1 + ((BREAKPOINT_LENGTH - 1) / sizeof(long)); i++) {
		long a =
		    ptrace(PTRACE_PEEKTEXT, pid, sbp->addr + i * sizeof(long),
			   0);
		for (j = 0;
		     j < sizeof(long)
		     && i * sizeof(long) + j < BREAKPOINT_LENGTH; j++) {
			unsigned char *bytes = (unsigned char *)&a;

			bytes[j] = sbp->orig_value[i * sizeof(long) + j];
		}
		ptrace(PTRACE_POKETEXT, pid, sbp->addr + i * sizeof(long), a);
	}
}
#endif				/* ARCH_HAVE_DISABLE_BREAKPOINT */
