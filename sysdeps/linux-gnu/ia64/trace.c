#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <string.h>
#include <asm/ptrace_offsets.h>
#include <asm/rse.h>

#include "ltrace.h"

/* What we think of as a bundle, ptrace thinks of it as two unsigned
 * longs */
union bundle_t {
       /* An IA64 instruction bundle has a 5 bit header describing the
        * type of bundle, then 3 41 bit instructions 
	*/
	struct {
		struct {
			unsigned long template  : 5 ;
			unsigned long slot0     : 41;
			unsigned long bot_slot1 : 18;
		} word0;
		struct {
			unsigned long top_slot1 : 23;
			unsigned long slot2     : 41;
		} word1;
	} bitmap;
	unsigned long code[2];
};

int
syscall_p(struct process * proc, int status, int * sysnum) {

	if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		unsigned long slot = (ptrace (PTRACE_PEEKUSER, proc->pid, PT_CR_IPSR, 0) >> 41) & 0x3;
		unsigned long ip = ptrace(PTRACE_PEEKUSER, proc->pid, PT_CR_IIP, 0);

		/* r15 holds the system call number */
		unsigned long r15 = ptrace (PTRACE_PEEKUSER, proc->pid, PT_R15, 0);
		unsigned long insn;

		union bundle_t bundle;

		/* On fault, the IP has moved forward to the next
		 * slot.  If that is zero, then the actual place we
		 * broke was in the previous bundle, so wind back the
		 * IP.
		 */
		if (slot == 0)
			ip = ip - 16;
		bundle.code[0] = ptrace (PTRACE_PEEKTEXT, proc->pid, ip, 0);
		bundle.code[1] = ptrace (PTRACE_PEEKTEXT, proc->pid, ip+8, 0);

		unsigned long bot = 0UL | bundle.bitmap.word0.bot_slot1;
		unsigned long top = 0UL | bundle.bitmap.word1.top_slot1;

		/* handle the rollback, slot 0 is actually slot 2 of
		 * the previous instruction (see above) */
		switch (slot) {
		case 0:
			insn = bundle.bitmap.word1.slot2;
			break;
		case 1:
			insn = bundle.bitmap.word0.slot0;
			break;
		case 2:
			/* make sure we're shifting about longs */
			insn =  0UL | bot | (top << 18UL);
			break;
		default:
			printf("Ummm, can't find instruction slot?\n");
			exit(1);
		}

		/* We need to support both the older break instruction
		 * type syscalls, and the new epc type ones.  
		 *
		 * Bit 20 of the break constant is encoded in the "i"
		 * bit (bit 36) of the instruction, hence you should
		 * see 0x1000000000.
		 *
		 *  An EPC call is just 0x1ffffffffff
		 */
		if (insn == 0x1000000000 || insn == 0x1ffffffffff) {
			*sysnum = r15;
			if (proc->callstack_depth > 0 &&
			    proc->callstack[proc->callstack_depth-1].is_syscall) {
				return 2;
			}
			return 1;
		}
	}
	return 0;
}

long
gimme_arg(enum tof type, struct process * proc, int arg_num) {
	
	unsigned long bsp, cfm;

	bsp = ptrace (PTRACE_PEEKUSER, proc->pid, PT_AR_BSP, 0);
	cfm = ptrace(PTRACE_PEEKUSER, proc->pid, PT_CFM, 0);

	if (arg_num==-1) 		/* return value */
		return ptrace (PTRACE_PEEKUSER, proc->pid, PT_R8, 0);
	
	/* First 8 arguments are passed in registers on the register
	 * stack, the following arguments are passed on the stack
	 * after a 16 byte scratch area
	 */
	if (type==LT_TOF_FUNCTION || LT_TOF_FUNCTIONR) {
		if (arg_num < 8)
			return ptrace (PTRACE_PEEKDATA, proc->pid, (long)ia64_rse_skip_regs((long*)bsp, -cfm+arg_num), 0);
		else {
			unsigned long sp = ptrace (PTRACE_PEEKUSER, proc->pid, PT_R12, 0) + 16;
			return ptrace (PTRACE_PEEKDATA, proc->pid, sp + (8*(arg_num - 8))); 
		}
	}
	
	if (type==LT_TOF_SYSCALL || LT_TOF_SYSCALLR ) 
		return ptrace (PTRACE_PEEKDATA, proc->pid, (long)ia64_rse_skip_regs((long *) bsp, arg_num), 0);

	/* error if we get here */
	fprintf(stderr, "gimme_arg called with wrong arguments\n");
	exit(1);
}

void
save_register_args(enum tof type, struct process * proc) {
}

void
get_arch_dep(struct process * proc) {
}
