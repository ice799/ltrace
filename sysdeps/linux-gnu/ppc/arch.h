#define BREAKPOINT_VALUE { 0x7f, 0xe0, 0x00, 0x08 }
#define BREAKPOINT_LENGTH 4
#define DECR_PC_AFTER_BREAK 0

#define LT_ELFCLASS	ELFCLASS32
#define LT_ELF_MACHINE	EM_PPC
#ifdef __powerpc64__ // Says 'ltrace' is 64 bits, says nothing about target.
#define LT_ELFCLASS2	ELFCLASS64
#define LT_ELF_MACHINE2	EM_PPC64
#endif

#define PLT_REINITALISATION_BP    "_start"

#define PPC_NOP { 0x60, 0x00, 0x00, 0x00 }
#define PPC_NOP_LENGTH 4

#if (PPC_NOP_LENGTH != BREAKPOINT_LENGTH)
#error "Length of the breakpoint value not equal to the length of a nop instruction"
#endif
