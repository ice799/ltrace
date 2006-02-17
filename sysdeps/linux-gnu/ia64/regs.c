#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ptrace.h>

#include <asm/ptrace_offsets.h> 
#include <asm/rse.h> 

#include <stddef.h>
#include "debug.h"
#include "ltrace.h"

void *
get_instruction_pointer(struct process * proc) {
	unsigned long ip = ptrace(PTRACE_PEEKUSER, proc->pid, PT_CR_IIP, 0);
	unsigned long slot = (ptrace(PTRACE_PEEKUSER, proc->pid, PT_CR_IPSR, 0) >> 41) & 3;
	
	return (void*)(ip | slot);
}

void
set_instruction_pointer(struct process * proc, void * addr) {

	unsigned long newip = (unsigned long)addr;
	int slot = (int) addr & 0xf;
	unsigned long psr = ptrace(PTRACE_PEEKUSER, proc->pid, PT_CR_IPSR, 0);
	
	psr &= ~(3UL << 41);
	psr |= (unsigned long)(slot & 0x3) << 41;
	
	newip &= ~0xfUL;

	ptrace(PTRACE_POKEUSER, proc->pid, PT_CR_IIP, (long)newip);
	ptrace(PTRACE_POKEUSER, proc->pid, PT_CR_IPSR, psr);
}

void *
get_stack_pointer(struct process * proc) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, PT_R12, 0);
}

void *
get_return_addr(struct process * proc, void * stack_pointer) {
	return (void *)ptrace(PTRACE_PEEKUSER, proc->pid, PT_B0, 0);
}
