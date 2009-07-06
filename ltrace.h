typedef enum Event_type Event_type;
enum Event_type {
	EVENT_NONE=0,
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
	EVENT_LIBCALL,
	EVENT_LIBRET,
	EVENT_NEW,        /* in this case, proc is NULL */
	EVENT_MAX
};

typedef struct Process Process;
typedef struct Event Event;
struct Event {
	Process * proc;
	Event_type type;
	union {
		int ret_val;     /* EVENT_EXIT */
		int signum;      /* EVENT_SIGNAL, EVENT_EXIT_SIGNAL */
		int sysnum;      /* EVENT_SYSCALL, EVENT_SYSRET */
		void * brk_addr; /* EVENT_BREAKPOINT */
		int newpid;      /* EVENT_CLONE, EVENT_NEW */
	} e_un;
};

typedef void (*callback_func) (Event *);

extern void ltrace_init(int argc, char **argv);
extern void ltrace_add_callback(callback_func f, Event_type type);
extern void ltrace_main(void);
