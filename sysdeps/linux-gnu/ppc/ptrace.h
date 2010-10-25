#include <sys/ptrace.h>
#include <sys/ucontext.h>
typedef struct {
  int valid;
  unsigned long func_args[6];
  unsigned long sysc_args[6];
  gregset_t regs;
  fpregset_t fpregs;
  gregset_t regs_copy;
  fpregset_t fpregs_copy;
} proc_archdep;
