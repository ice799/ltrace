#undef PTRACE_GETREGS
#undef PTRACE_SETREGS
#undef PTRACE_GETFPREGS
#undef PTRACE_SETFPREGS
#include <sys/ptrace.h>
#ifndef PTRACE_SUNDETACH
#define PTRACE_SUNDETACH 11
#endif
#undef PT_DETACH
#undef PTRACE_DETACH
#define PT_DETACH PTRACE_SUNDETACH
#define PTRACE_DETACH PTRACE_SUNDETACH

#include <asm/reg.h>
#include "ltrace.h"

typedef struct {
	int valid;
	struct regs regs;
	unsigned int func_arg[6];
	unsigned int sysc_arg[6];
} proc_archdep;
