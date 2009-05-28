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

#include "main.h"

/* What we think of as a bundle, ptrace thinks of it as two unsigned
 * longs */
union bundle_t {
	/* An IA64 instruction bundle has a 5 bit header describing the
	 * type of bundle, then 3 41 bit instructions
	 */
	struct {
		struct {
			unsigned long template:5;
			unsigned long slot0:41;
			unsigned long bot_slot1:18;
		} word0;
		struct {
			unsigned long top_slot1:23;
			unsigned long slot2:41;
		} word1;
	} bitmap;
	unsigned long code[2];
};

union cfm_t {
	struct {
		unsigned long sof:7;
		unsigned long sol:7;
		unsigned long sor:4;
		unsigned long rrb_gr:7;
		unsigned long rrb_fr:7;
		unsigned long rrb_pr:6;
	} cfm;
	unsigned long value;
};

int
syscall_p(Process *proc, int status, int *sysnum) {
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
		unsigned long slot =
		    (ptrace(PTRACE_PEEKUSER, proc->pid, PT_CR_IPSR, 0) >> 41) &
		    0x3;
		unsigned long ip =
		    ptrace(PTRACE_PEEKUSER, proc->pid, PT_CR_IIP, 0);

		/* r15 holds the system call number */
		unsigned long r15 =
		    ptrace(PTRACE_PEEKUSER, proc->pid, PT_R15, 0);
		unsigned long insn;

		union bundle_t bundle;

		/* On fault, the IP has moved forward to the next
		 * slot.  If that is zero, then the actual place we
		 * broke was in the previous bundle, so wind back the
		 * IP.
		 */
		if (slot == 0)
			ip = ip - 16;
		bundle.code[0] = ptrace(PTRACE_PEEKTEXT, proc->pid, ip, 0);
		bundle.code[1] = ptrace(PTRACE_PEEKTEXT, proc->pid, ip + 8, 0);

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
			insn = 0UL | bot | (top << 18UL);
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
				proc->callstack[proc->callstack_depth - 1].is_syscall &&
				proc->callstack[proc->callstack_depth - 1].c_un.syscall == *sysnum) {
				return 2;
			}
			return 1;
		}
	}
	return 0;
}

/* Stolen from David Mosberger's utrace tool, which he released under
   the GPL
   (http://www.gelato.unsw.edu.au/archives/linux-ia64/0104/1405.html) */
static inline double
fpreg_to_double (struct ia64_fpreg *fp) {
  double result;

  asm ("ldf.fill %0=%1" : "=f"(result) : "m"(*fp));
  return result;
}

static long
gimme_long_arg(enum tof type, Process *proc, int arg_num) {
	union cfm_t cfm;
	unsigned long bsp;

	bsp = ptrace(PTRACE_PEEKUSER, proc->pid, PT_AR_BSP, 0);
	cfm.value = ptrace(PTRACE_PEEKUSER, proc->pid, PT_CFM, 0);

	if (arg_num == -1)	/* return value */
		return ptrace(PTRACE_PEEKUSER, proc->pid, PT_R8, 0);

	/* First 8 arguments are passed in registers on the register
	 * stack, the following arguments are passed on the stack
	 * after a 16 byte scratch area
	 *
	 * If the function has returned, the ia64 register window has
	 * been reverted to the caller's configuration. So although in
	 * the callee, the first parameter is in R32, in the caller
	 * the first parameter comes in the registers after the local
	 * registers (really, input parameters plus locals, but the
	 * hardware doesn't track the distinction.) So we have to add
	 * in the size of the local area (sol) to find the first
	 * parameter passed to the callee. */
	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		if (arg_num < 8) {
                        if (type == LT_TOF_FUNCTIONR)
				arg_num += cfm.cfm.sol;

			return ptrace(PTRACE_PEEKDATA, proc->pid,
				      (long)ia64_rse_skip_regs((unsigned long *)bsp,
							       -cfm.cfm.sof + arg_num),
				      0);
		} else {
			unsigned long sp =
			    ptrace(PTRACE_PEEKUSER, proc->pid, PT_R12, 0) + 16;
			return ptrace(PTRACE_PEEKDATA, proc->pid,
				      sp + (8 * (arg_num - 8)));
		}
	}

	if (type == LT_TOF_SYSCALL || LT_TOF_SYSCALLR)
		return ptrace(PTRACE_PEEKDATA, proc->pid,
			      (long)ia64_rse_skip_regs((unsigned long *)bsp, arg_num),
			      0);

	/* error if we get here */
	fprintf(stderr, "gimme_arg called with wrong arguments\n");
	exit(1);
}

static long float_regs[8] = { PT_F8, PT_F9, PT_F10, PT_F11,
			      PT_F12, PT_F13, PT_F14, PT_F15 };
static double
gimme_float_arg(enum tof type, Process *proc, int arg_num) {
	union cfm_t cfm;
	unsigned long bsp;
	struct ia64_fpreg reg;

	if (arg_num == -1) {	/* return value */
		reg.u.bits[0] = ptrace(PTRACE_PEEKUSER, proc->pid,
				       PT_F8, 0);
		reg.u.bits[1] = ptrace(PTRACE_PEEKUSER, proc->pid,
				       PT_F8 + 0x8, 0);
		return fpreg_to_double(&reg);
	}

	bsp = ptrace(PTRACE_PEEKUSER, proc->pid, PT_AR_BSP, 0);
	cfm.value = ptrace(PTRACE_PEEKUSER, proc->pid, PT_CFM, 0);

	/* The first 8 arguments are passed in regular registers
	 * (counting from R32), unless they are floating point values
	 * (the case in question here). In that case, up to the first
	 * 8 regular registers are still "allocated" for each of the
	 * first 8 parameters, but if a parameter is floating point,
	 * then the register is left unset and the parameter is passed
	 * in the first available floating-point register, counting
	 * from F8.
	 *
	 * Take func(int a, float f, int b, double d), for example.
	 *    a - passed in R32
	 *    f - R33 left unset, value passed in F8
	 *    b - passed in R34
	 *    d - R35 left unset, value passed in F9
	 *
	 * ltrace handles this by counting floating point arguments
	 * while parsing declarations. The "arg_num" in this routine
	 * (which is only called for floating point values) really
	 * means which floating point parameter we're looking for,
	 * ignoring everything else.
	 *
	 * Following the first 8 arguments, the remaining arguments
	 * are passed on the stack after a 16 byte scratch area
	 */
	if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
		if (arg_num < 8) {
			reg.u.bits[0] = ptrace(PTRACE_PEEKUSER, proc->pid,
					       float_regs[arg_num], 0);
			reg.u.bits[1] = ptrace(PTRACE_PEEKUSER, proc->pid,
					       float_regs[arg_num] + 0x8, 0);
			return fpreg_to_double(&reg);
		} else {
			unsigned long sp =
			    ptrace(PTRACE_PEEKUSER, proc->pid, PT_R12, 0) + 16;
			reg.u.bits[0] = ptrace(PTRACE_PEEKDATA, proc->pid,
					       sp + (8 * (arg_num - 8)));
			reg.u.bits[0] = ptrace(PTRACE_PEEKDATA, proc->pid,
					       sp + (8 * (arg_num - 8)) + 0x8);
			return fpreg_to_double(&reg);
		}
	}

	/* error if we get here */
	fprintf(stderr, "gimme_arg called with wrong arguments\n");
	exit(1);
}

long
gimme_arg(enum tof type, Process *proc, int arg_num, arg_type_info *info) {
	union {
		long l;
		float f;
		double d;
	} cvt;

	if (info->type == ARGTYPE_FLOAT)
		cvt.f = gimme_float_arg(type, proc, info->u.float_info.float_index);
	else if (info->type == ARGTYPE_DOUBLE)
		cvt.d = gimme_float_arg(type, proc, info->u.double_info.float_index);
	else
		cvt.l = gimme_long_arg(type, proc, arg_num);

	return cvt.l;
}

void
save_register_args(enum tof type, Process *proc) {
}

void
get_arch_dep(Process *proc) {
}
