typedef struct Process Process;
typedef struct Event Event;
struct Event {
	Process *proc;
	enum {
		EVENT_NONE,
		EVENT_SIGNAL,
		EVENT_EXIT,
		EVENT_EXIT_SIGNAL,
		EVENT_SYSCALL,
		EVENT_SYSRET,
		EVENT_ARCH_SYSCALL,
		EVENT_ARCH_SYSRET,
		EVENT_CLONE,
		EVENT_EXEC,
		EVENT_BREAKPOINT,
		EVENT_NEW       /* in this case, proc is NULL */
	} type;
	union {
		int ret_val;    /* EVENT_EXIT */
		int signum;     /* EVENT_SIGNAL, EVENT_EXIT_SIGNAL */
		int sysnum;     /* EVENT_SYSCALL, EVENT_SYSRET */
		void *brk_addr; /* EVENT_BREAKPOINT */
		int newpid;     /* EVENT_CLONE, EVENT_NEW */
	} e_un;
};

extern void ltrace_init(int argc, char **argv);
extern void ltrace_add_callback(void (*func)(Event *));
extern void ltrace_main(void);
