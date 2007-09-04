#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include "debug.h"
#include "ltrace.h"
#include "mipsel.h"
#if (!defined(PTRACE_PEEKUSER) && defined(PTRACE_PEEKUSR))
# define PTRACE_PEEKUSER PTRACE_PEEKUSR
#endif

#if (!defined(PTRACE_POKEUSER) && defined(PTRACE_POKEUSR))
# define PTRACE_POKEUSER PTRACE_POKEUSR
#endif


/**
   \addtogroup mipsel Mipsel specific functions.

   These are the functions that it looks like I need to implement in
   order to get ltrace to work on our target. 

   @{
 */

/**
   \param proc The process that had an event. 

   Called by \c wait_for_something() right after the return from wait.

   Most targets just return here. A couple use proc->arch_ptr for a
   private data area.
 */
void get_arch_dep(struct process *proc)
{

}

/**
   \param proc Process that had event. 
   \param status From \c wait()
   \param sysnum 0-based syscall number. 
   \return 1 if syscall, 2 if sysret, 0 otherwise.

   Called by \c wait_for_something() after the call to get_arch_dep()

   It seems that the ptrace call trips twice on a system call, once
   just before the system call and once when it returns. Both times,
   the pc points at the instruction just after the mipsel "syscall"
   instruction.

   There are several possiblities for system call sets, each is offset
   by a base from the others. On our system, it looks like the base
   for the system calls is 4000.
 */
int syscall_p(struct process *proc, int status, int *sysnum)
{
	if (WIFSTOPPED(status)
	    && WSTOPSIG(status) == (SIGTRAP | proc->tracesysgood)) {
       /* get the user's pc (plus 8) */
       long pc = (long)get_instruction_pointer(proc);
       /* fetch the SWI instruction */
       int insn = ptrace(PTRACE_PEEKTEXT, proc->pid, pc - 4, 0);
       int num = ptrace(PTRACE_PEEKTEXT, proc->pid, pc - 8, 0);
       
/*
  On a mipsel,  syscall looks like:
  24040fa1    li v0, 0x0fa1   # 4001 --> _exit syscall
  0000000c    syscall
 */
      if(insn!=0x0000000c){
          return 0;
      }

      *sysnum = (num & 0xFFFF) - 4000;
      /* if it is a syscall, return 1 or 2 */
      if (proc->callstack_depth > 0 &&
          proc->callstack[proc->callstack_depth - 1].is_syscall) {
          return 2;
      }
      
      if (*sysnum >= 0) {
          return 1;
      }
   }
	return 0;
}
/**
   \param type Function/syscall call or return.
   \param proc The process that had an event.
   \param arg_num -1 for return value, 
   \return The argument to fetch. 

   A couple of assumptions. 

-  Type is LT_TOF_FUNCTIONR or LT_TOF_SYSCALLR if arg_num==-1. These
   types are only used in calls for output_right(), which only uses -1
   for arg_num.
-  Type is LT_TOF_FUNCTION or LT_TOF_SYSCALL for args 0...4. 
-   I'm only displaying the first 4 args (Registers a0..a3). Good
   enough for now.

  Mipsel conventions seem to be:
- syscall parameters: r4...r9
- syscall return: if(!a3){ return v0;} else{ errno=v0;return -1;}
- function call: r4..r7. Not sure how to get arg number 5. 
- function return: v0

The argument registers are wiped by a call, so it is a mistake to ask
for arguments on a return. If ltrace does this, we will need to cache
arguments somewhere on the call.

I'm not doing any floating point support here. 

*/
long gimme_arg(enum tof type, struct process *proc, int arg_num, arg_type_info *info)
{
    long ret;
    debug(2,"type %d arg %d",type,arg_num);
    if (type == LT_TOF_FUNCTION || type == LT_TOF_SYSCALL){
        if(arg_num <4){
            ret=ptrace(PTRACE_PEEKUSER,proc->pid,off_a0+arg_num,0);
            debug(2,"ret = %#lx",ret);
            return ret;
        } else {
            // If we need this, I think we can look at [sp+16] for arg_num==4.
            CP;
            return 0;
        }
    } 
    if(arg_num>=0){
       fprintf(stderr,"args on return?");
    }
    if(type == LT_TOF_FUNCTIONR) {
        return  ptrace(PTRACE_PEEKUSER,proc->pid,off_v0,0);
    }
    if (type == LT_TOF_SYSCALLR) {
        unsigned a3=ptrace(PTRACE_PEEKUSER, proc->pid,off_a3,0);
        unsigned v0=ptrace(PTRACE_PEEKUSER, proc->pid,off_v0,0);
        if(!a3){
            return v0;
        }
        return -1;
    }
    fprintf(stderr, "gimme_arg called with wrong arguments\n");
	return 0;
}

/**
   \param type Type of call/return
   \param proc Process to work with. 
   
   Called by \c output_left(), which is called on a syscall or
   function.
   
   The other architectures stub this out, but seems to be the place to
   stash off the arguments on a call so we have them on the return.
   
*/
void save_register_args(enum tof type, struct process *proc)
{
}

/**@}*/
