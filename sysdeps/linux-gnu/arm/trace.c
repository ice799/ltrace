#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "ltrace.h"

#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif

/* syscall tracing protocol: ArmLinux
 on the way in, ip is 0
 on the way out, ip is non-zero
*/
#define off_r0 0
#define off_ip 48
#define off_pc 60

void
get_arch_dep(struct process * proc) {
}

/* Returns 1 if syscall, 2 if sysret, 0 otherwise.
 */
int
syscall_p(struct process * proc, int status, int * sysnum) {
	if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGTRAP) {
		/* get the user's pc (plus 8) */
		int pc = ptrace(PTRACE_PEEKUSER, proc->pid, off_pc, 0);
		/* fetch the SWI instruction */
		int insn = ptrace(PTRACE_PEEKTEXT, proc->pid, pc-4, 0) ;
        
		*sysnum = insn & 0xFFFF;
		/* if it is a syscall, return 1 or 2 */
		if ((insn & 0xFFFF0000) == 0xef900000) {
			return ptrace(PTRACE_PEEKUSER, proc->pid, off_ip, 0) ? 2 : 1;
		}
	}
	return 0;
}
            
long
gimme_arg(enum tof type, struct process * proc, int arg_num) {
	if (arg_num==-1) {		/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, off_r0, 0);
	}

	/* deal with the ARM calling conventions */
	if (type==LT_TOF_FUNCTION || type==LT_TOF_FUNCTIONR) {
		if (arg_num<4) {
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4*arg_num, 0);
		} else {
			return ptrace(PTRACE_PEEKDATA, proc->pid, proc->stack_pointer+4*(arg_num-4), 0);
		}
	} else if (type==LT_TOF_SYSCALL || type==LT_TOF_SYSCALLR) {
		if (arg_num<5) {
			return ptrace(PTRACE_PEEKUSER, proc->pid, 4*arg_num, 0);
		} else {
			return ptrace(PTRACE_PEEKDATA, proc->pid, proc->stack_pointer+4*(arg_num-5), 0);
		}
	} else {
		fprintf(stderr, "gimme_arg called with wrong arguments\n");
		exit(1);
	}

	return 0;
}

void
save_register_args(enum tof type, struct process * proc) {
}
