#define BREAKPOINT_VALUE {0xcc}
#define BREAKPOINT_LENGTH 1
#define DECR_PC_AFTER_BREAK 1

#define LT_ELFCLASS	ELFCLASS64
#define LT_ELF_MACHINE	EM_X86_64
#define LT_ELFCLASS2	ELFCLASS32
#define LT_ELF_MACHINE2	EM_386

/* __NR_fork, __NR_clone, __NR_clone2, __NR_vfork and __NR_execve
   from asm-i386/unistd.h.  */
#define FORK_EXEC_SYSCALLS , { 2, 120, -1, 190, 11 }
