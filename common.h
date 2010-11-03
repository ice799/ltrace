#if defined(HAVE_LIBUNWIND)
#include <libunwind.h>
#endif /* defined(HAVE_LIBUNWIND) */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>

#include "ltrace.h"
#include "defs.h"
#include "dict.h"
#include "sysdep.h"
#include "debug.h"
#include "ltrace-elf.h"
#include "read_config_file.h"

#if defined HAVE_LIBIBERTY || defined HAVE_LIBSUPC__
# define USE_DEMANGLE
#endif

extern char * command;

extern int exiting;  /* =1 if we have to exit ASAP */

typedef struct Breakpoint Breakpoint;
struct Breakpoint {
	void * addr;
	unsigned char orig_value[BREAKPOINT_LENGTH];
	int enabled;
	struct library_symbol * libsym;
#ifdef __arm__
	int thumb_mode;
#endif
};

enum arg_type {
	ARGTYPE_UNKNOWN = -1,
	ARGTYPE_VOID,
	ARGTYPE_INT,
	ARGTYPE_UINT,
	ARGTYPE_LONG,
	ARGTYPE_ULONG,
	ARGTYPE_OCTAL,
	ARGTYPE_CHAR,
	ARGTYPE_SHORT,
	ARGTYPE_USHORT,
	ARGTYPE_FLOAT,		/* float value, may require index */
	ARGTYPE_DOUBLE,		/* double value, may require index */
	ARGTYPE_ADDR,
	ARGTYPE_FILE,
	ARGTYPE_FORMAT,		/* printf-like format */
	ARGTYPE_STRING,		/* NUL-terminated string */
	ARGTYPE_STRING_N,	/* String of known maxlen */
	ARGTYPE_ARRAY,		/* Series of values in memory */
	ARGTYPE_ENUM,		/* Enumeration */
	ARGTYPE_STRUCT,		/* Structure of values */
	ARGTYPE_POINTER,	/* Pointer to some other type */
	ARGTYPE_COUNT		/* number of ARGTYPE_* values */
};

typedef struct arg_type_info_t {
	enum arg_type type;
	union {
		/* ARGTYPE_ENUM */
		struct {
			size_t entries;
			char ** keys;
			int * values;
		} enum_info;

		/* ARGTYPE_ARRAY */
		struct {
			struct arg_type_info_t * elt_type;
			size_t elt_size;
			int len_spec;
		} array_info;

		/* ARGTYPE_STRING_N */
		struct {
			int size_spec;
		} string_n_info;

		/* ARGTYPE_STRUCT */
		struct {
			struct arg_type_info_t ** fields;	/* NULL-terminated */
			size_t * offset;
			size_t size;
		} struct_info;

		/* ARGTYPE_POINTER */
		struct {
			struct arg_type_info_t * info;
		} ptr_info;

		/* ARGTYPE_FLOAT */
		struct {
			size_t float_index;
		} float_info;

		/* ARGTYPE_DOUBLE */
		struct {
			size_t float_index;
		} double_info;
	} u;
} arg_type_info;

enum tof {
	LT_TOF_NONE = 0,
	LT_TOF_FUNCTION,	/* A real library function */
	LT_TOF_FUNCTIONR,	/* Return from a real library function */
	LT_TOF_SYSCALL,		/* A syscall */
	LT_TOF_SYSCALLR,	/* Return from a syscall */
	LT_TOF_STRUCT		/* Not a function; read args from struct */
};

typedef struct Function Function;
struct Function {
	const char * name;
	arg_type_info * return_info;
	int num_params;
	arg_type_info * arg_info[MAX_ARGS];
	int params_right;
	Function * next;
};

enum toplt {
	LS_TOPLT_NONE = 0,	/* PLT not used for this symbol. */
	LS_TOPLT_EXEC,		/* PLT for this symbol is executable. */
	LS_TOPLT_POINT		/* PLT for this symbol is a non-executable. */
};

extern Function * list_of_functions;
extern char *PLTs_initialized_by_here;

struct library_symbol {
	char * name;
	void * enter_addr;
	char needs_init;
	enum toplt plt_type;
	char is_weak;
	struct library_symbol * next;
};

struct callstack_element {
	union {
		int syscall;
		struct library_symbol * libfunc;
	} c_un;
	int is_syscall;
	void * return_addr;
	struct timeval time_spent;
	void * arch_ptr;
};

#define MAX_CALLDEPTH 64

typedef enum Process_State Process_State;
enum Process_State {
	STATE_ATTACHED = 0,
	STATE_BEING_CREATED,
	STATE_IGNORED  /* ignore this process (it's a fork and no -f was used) */
};

struct Process {
	Process_State state;
	Process * parent;         /* needed by STATE_BEING_CREATED */
	char * filename;
	pid_t pid;
	Dict * breakpoints;
	int breakpoints_enabled;  /* -1:not enabled yet, 0:disabled, 1:enabled */
	int mask_32bit;           /* 1 if 64-bit ltrace is tracing 32-bit process */
	unsigned int personality;
	int tracesysgood;         /* signal indicating a PTRACE_SYSCALL trap */

	int callstack_depth;
	struct callstack_element callstack[MAX_CALLDEPTH];
	struct library_symbol * list_of_symbols;

	/* Arch-dependent: */
	void * instruction_pointer;
	void * stack_pointer;      /* To get return addr, args... */
	void * return_addr;
	Breakpoint * breakpoint_being_enabled;
	void * arch_ptr;
	short e_machine;
	short need_to_reinitialize_breakpoints;
#ifdef __arm__
	int thumb_mode;           /* ARM execution mode: 0: ARM, 1: Thumb */
#endif

	/* output: */
	enum tof type_being_displayed;

#if defined(HAVE_LIBUNWIND)
	/* libunwind address space */
	unw_addr_space_t unwind_as;
	void *unwind_priv;
#endif /* defined(HAVE_LIBUNWIND) */

	Process * next;
};

struct opt_c_struct {
	int count;
	struct timeval tv;
};

#include "options.h"
#include "output.h"
#ifdef USE_DEMANGLE
#include "demangle.h"
#endif

extern Dict * dict_opt_c;

extern Process * list_of_processes;

extern Event * next_event(void);
extern Process * pid2proc(pid_t pid);
extern void handle_event(Event * event);
extern void execute_program(Process *, char **);
extern int display_arg(enum tof type, Process * proc, int arg_num, arg_type_info * info);
extern Breakpoint * address2bpstruct(Process * proc, void * addr);
extern void breakpoints_init(Process * proc);
extern void insert_breakpoint(Process * proc, void * addr, struct library_symbol * libsym);
extern void delete_breakpoint(Process * proc, void * addr);
extern void enable_all_breakpoints(Process * proc);
extern void disable_all_breakpoints(Process * proc);
extern void reinitialize_breakpoints(Process *);

extern Process * open_program(char * filename, pid_t pid);
extern void open_pid(pid_t pid);
extern void show_summary(void);
extern arg_type_info * lookup_prototype(enum arg_type at);

/* Arch-dependent stuff: */
extern char * pid2name(pid_t pid);
extern void trace_set_options(Process * proc, pid_t pid);
extern void trace_me(void);
extern int trace_pid(pid_t pid);
extern void untrace_pid(pid_t pid);
extern void get_arch_dep(Process * proc);
extern void * get_instruction_pointer(Process * proc);
extern void set_instruction_pointer(Process * proc, void * addr);
extern void * get_stack_pointer(Process * proc);
extern void * get_return_addr(Process * proc, void * stack_pointer);
extern void set_return_addr(Process * proc, void * addr);
extern void enable_breakpoint(pid_t pid, Breakpoint * sbp);
extern void disable_breakpoint(pid_t pid, const Breakpoint * sbp);
extern int syscall_p(Process * proc, int status, int * sysnum);
extern void continue_process(pid_t pid);
extern void continue_after_signal(pid_t pid, int signum);
extern void continue_after_breakpoint(Process * proc, Breakpoint * sbp);
extern void continue_enabling_breakpoint(pid_t pid, Breakpoint * sbp);
extern long gimme_arg(enum tof type, Process * proc, int arg_num, arg_type_info * info);
extern void save_register_args(enum tof type, Process * proc);
extern int umovestr(Process * proc, void * addr, int len, void * laddr);
extern int umovelong (Process * proc, void * addr, long * result, arg_type_info * info);
extern int ffcheck(void * maddr);
extern void * sym2addr(Process *, struct library_symbol *);

#if 0				/* not yet */
extern int umoven(Process * proc, void * addr, int len, void * laddr);
#endif
