#include <sys/ptrace.h>
#include <asm/ptrace.h>

typedef struct {
	int valid;
	struct pt_regs regs;
	long func_arg[5];
	long sysc_arg[5];
} proc_archdep;
