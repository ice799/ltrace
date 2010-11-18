#include <sys/ptrace.h>
#include <sys/ucontext.h>

#ifdef __powerpc64__
#define GET_FPREG(RS, R) ((RS)[R])
#else
#define GET_FPREG(RS, R) ((RS).fpregs[R])
#endif

typedef struct {
  int valid;
  gregset_t regs;
  fpregset_t fpregs;
  gregset_t regs_copy;
  fpregset_t fpregs_copy;
} proc_archdep;
