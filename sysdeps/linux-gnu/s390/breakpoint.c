/*
** S/390 version
** (C) Copyright 2001 IBM Poughkeepsie, IBM Corporation
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ptrace.h>
#include "ltrace.h"
#include "s390/arch.h"

void
enable_breakpoint(pid_t pid, struct breakpoint * sbp) {
	long a;
	long mask1;
	long mask2;
	int shift;
	int i;

	mask1 = 0xFF000000;
	mask2 = 0x00000000;
	shift = 24;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);

	for( i=0; i < BREAKPOINT_LENGTH; i++ ) {
		sbp->orig_value[i] = (a & mask1) >> shift;
		mask2 |= mask1;
		mask1 = (mask1 >> 8) & 0x00FFFFFF;
		shift -= 8;
	}
	mask2 = ~mask2;

	a &= mask2;
	a |= BREAKPOINT_VALUE;

	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}

void
disable_breakpoint(pid_t pid, const struct breakpoint * sbp) {
	long a;
	long b;
	long mask1;
	long mask2;
	int shift;
	int i;

	b = 0x00000000;
	mask1 = 0xFF000000;
	mask2 = 0x00000000;
	shift = 24;

	a = ptrace(PTRACE_PEEKTEXT, pid, sbp->addr, 0);

	for( i=0; i < BREAKPOINT_LENGTH; i++ ) {
		b |= sbp->orig_value[i] << shift;
		mask2 |= mask1;
		mask1 = (mask1 >> 8) & 0x00FFFFFF;
		shift -= 8;
	}
	mask2 = ~mask2;

	a &= mask2;
	a |= b;

	ptrace(PTRACE_POKETEXT, pid, sbp->addr, a);
}
