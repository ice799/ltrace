/*
** S/390 version
** Copyright (C) 2001 IBM Poughkeepsie, IBM Corporation
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

#ifdef __s390x__
#define PSW_MASK	0xffffffffffffffff
#define PSW_MASK31	0x7fffffff
#else
#define PSW_MASK	0x7fffffff
#endif

void *
get_instruction_pointer(Process *proc) {
	long ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_PSWADDR, 0) & PSW_MASK;
#ifdef __s390x__
	if (proc->mask_32bit)
		ret &= PSW_MASK31;
#endif
	return (void *)ret;
}

void
set_instruction_pointer(Process *proc, void *addr) {
#ifdef __s390x__
	if (proc->mask_32bit)
		addr = (void *)((long)addr & PSW_MASK31);
#endif
	ptrace(PTRACE_POKEUSER, proc->pid, PT_PSWADDR, addr);
}

void *
get_stack_pointer(Process *proc) {
	long ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR15, 0) & PSW_MASK;
#ifdef __s390x__
	if (proc->mask_32bit)
		ret &= PSW_MASK31;
#endif
	return (void *)ret;
}

void *
get_return_addr(Process *proc, void *stack_pointer) {
	long ret = ptrace(PTRACE_PEEKUSER, proc->pid, PT_GPR14, 0) & PSW_MASK;
#ifdef __s390x__
	if (proc->mask_32bit)
		ret &= PSW_MASK31;
#endif
	return (void *)ret;
}
