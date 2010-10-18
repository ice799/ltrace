/*
** S/390 version
** (C) Copyright 2001 IBM Poughkeepsie, IBM Corporation
*/

#define BREAKPOINT_VALUE { 0x00, 0x01 }
#define BREAKPOINT_LENGTH 2
#define DECR_PC_AFTER_BREAK 2

#ifdef __s390x__
#define LT_ELFCLASS	ELFCLASS64
#define LT_ELF_MACHINE	EM_S390
#define LT_ELFCLASS2	ELFCLASS32
#define LT_ELF_MACHINE2	EM_S390

/* __NR_fork, __NR_clone, __NR_clone2, __NR_vfork and __NR_execve
   from asm-s390/unistd.h.  */
#define FORK_EXEC_SYSCALLS , { 2, 120, -1, 190, 11 }

#else
#define LT_ELFCLASS	ELFCLASS32
#define LT_ELF_MACHINE	EM_S390
#endif
