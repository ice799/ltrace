#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/user.h>

typedef struct {
	int valid;
	struct user_regs_struct regs;
	struct user_fpregs_struct fpregs;
} proc_archdep;

typedef struct {
	struct user_regs_struct regs_copy;
	struct user_fpregs_struct fpregs_copy;
} callstack_achdep;
