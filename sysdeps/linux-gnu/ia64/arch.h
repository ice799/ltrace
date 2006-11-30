#define ARCH_HAVE_DISABLE_BREAKPOINT 1
#define ARCH_HAVE_ENABLE_BREAKPOINT 1

#define BREAKPOINT_LENGTH 16
#define BREAKPOINT_VALUE {0}
#define DECR_PC_AFTER_BREAK 0

#define LT_ELFCLASS   ELFCLASS64
#define LT_ELF_MACHINE EM_IA_64

// ia64 actually does use .opd, but we don't need to do the
// translation manually.
#undef ARCH_SUPPORTS_OPD
