/*
** S/390 version
** (C) Copyright 2001 IBM Poughkeepsie, IBM Corporation
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

void *
get_instruction_pointer(pid_t pid) {
	return (void *)(ptrace(PTRACE_PEEKUSER, pid, PT_PSWADDR, 0) & 0x7fffffff);
}

void
set_instruction_pointer(pid_t pid, long addr) {
	ptrace(PTRACE_POKEUSER, pid, PT_PSWADDR, addr);
}

void *
get_stack_pointer(pid_t pid) {
	return (void *)ptrace(PTRACE_PEEKUSER, pid, PT_GPR15, 0);
}

void *
get_return_addr(pid_t pid, void * stack_pointer) {
	return (void *)(ptrace(PTRACE_PEEKUSER, pid, PT_GPR14, 0) & 0x7fffffff);
}
