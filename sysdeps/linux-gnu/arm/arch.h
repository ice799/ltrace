#define BREAKPOINT_VALUE { 0x01, 0x00, 0x9f, 0xef }
#define BREAKPOINT_LENGTH 4
/* we don't need to decr the pc; the kernel does it for us! */
#define DECR_PC_AFTER_BREAK 0
