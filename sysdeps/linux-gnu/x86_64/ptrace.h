#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/user.h>

typedef struct {
	int valid;
	unsigned long func_args[6];
	uint32_t func_fpr_args[64];
	unsigned long sysc_args[6];
	struct user_fpregs_struct fpregs;
	struct user_regs_struct regs;
} proc_archdep;

