#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include "arch.h"
#include "options.h"
#include "output.h"

static unsigned char break_insn[] = BREAKPOINT_VALUE;

void
enable_breakpoint(pid_t pid, struct breakpoint * sbp) {
	int i,j;

	if (opt_d>1) {
		output_line(0, "enable_breakpoint(%d,%p)", pid, sbp->addr);
	}

	for(i=0; i < 1+((BREAKPOINT_LENGTH-1)/sizeof(long)); i++) {
		long a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr + i*sizeof(long), 0);
		for(j=0; j<sizeof(long) && i*sizeof(long)+j < BREAKPOINT_LENGTH; j++) {
			unsigned char * bytes = (unsigned char *)&a;

			sbp->orig_value[i*sizeof(long)+j] = bytes[i*sizeof(long)+j];
			bytes[i*sizeof(long)+j] = break_insn[i*sizeof(long)+j];
		}
		ptrace(PTRACE_POKETEXT, pid, sbp->addr + i*sizeof(long), a);
	}
}

void
disable_breakpoint(pid_t pid, const struct breakpoint * sbp) {
	int i,j;

	if (opt_d>1) {
		output_line(0, "disable_breakpoint(%d,%p)", pid, sbp->addr);
	}

	for(i=0; i < 1+((BREAKPOINT_LENGTH-1)/sizeof(long)); i++) {
		long a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr + i*sizeof(long), 0);
		for(j=0; j<sizeof(long) && i*sizeof(long)+j < BREAKPOINT_LENGTH; j++) {
			unsigned char * bytes = (unsigned char *)&a;

			bytes[i*sizeof(long)+j] = sbp->orig_value[i*sizeof(long)+j];
		}
		ptrace(PTRACE_POKETEXT, pid, sbp->addr + i*sizeof(long), a);
	}
}
