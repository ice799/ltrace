#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"

#if HAVE_LIBIBERTY
#include "demangle.h"
#endif

static pid_t current_pid = 0;
static int current_depth = 0;
static int current_column = 0;

static void
output_indent(struct process * proc) {
	current_column += fprintf(output, "%*s", opt_n * proc->callstack_depth, "");
}

static void
begin_of_line(enum tof type, struct process * proc) {
	current_column = 0;
	if (!proc) {
		return;
	}
	if ((output!=stderr) && (opt_p || opt_f)) {
		current_column += fprintf(output, "%u ", proc->pid);
	} else if (list_of_processes->next) {
		current_column += fprintf(output, "[pid %u] ", proc->pid);
	}
	if (opt_r) {
		struct timeval tv;
		struct timezone tz;
		static struct timeval old_tv={0,0};
		struct timeval diff;

		gettimeofday(&tv, &tz);

		if (old_tv.tv_sec==0 && old_tv.tv_usec==0) {
			old_tv.tv_sec=tv.tv_sec;
			old_tv.tv_usec=tv.tv_usec;
		}
		diff.tv_sec = tv.tv_sec - old_tv.tv_sec;
		if (tv.tv_usec >= old_tv.tv_usec) {
			diff.tv_usec = tv.tv_usec - old_tv.tv_usec;
		} else {
			diff.tv_sec++;
			diff.tv_usec = 1000000 + tv.tv_usec - old_tv.tv_usec;
		}
		old_tv.tv_sec = tv.tv_sec;
		old_tv.tv_usec = tv.tv_usec;
		current_column += fprintf(output, "%3lu.%06d ",
			diff.tv_sec, (int)diff.tv_usec);
	}
	if (opt_t) {
		struct timeval tv;
		struct timezone tz;

		gettimeofday(&tv, &tz);
		if (opt_t>2) {
			current_column += fprintf(output, "%lu.%06d ",
				tv.tv_sec, (int)tv.tv_usec);
		} else if (opt_t>1) {
			struct tm * tmp = localtime(&tv.tv_sec);
			current_column += fprintf(output, "%02d:%02d:%02d.%06d ",
				tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (int)tv.tv_usec);
		} else {
			struct tm * tmp = localtime(&tv.tv_sec);
			current_column += fprintf(output, "%02d:%02d:%02d ",
				tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
		}
	}
	if (opt_i) {
		if (type==LT_TOF_FUNCTION) {
			current_column += fprintf(output, "[%08x] ",
				(unsigned)proc->return_addr);
		} else {
			current_column += fprintf(output, "[%08x] ",
				(unsigned)proc->instruction_pointer);
		}
	}
	if (opt_n > 0 && type!=LT_TOF_NONE) {
		output_indent(proc);
	}
}

static struct function *
name2func(char * name) {
	struct function * tmp;
	const char * str1, * str2;

	tmp = list_of_functions;
	while(tmp) {
#if HAVE_LIBIBERTY
		str1 = opt_C ? my_demangle(tmp->name) : tmp->name;
		str2 = opt_C ? my_demangle(name) : name;
#else
		str1 = tmp->name;
		str2 = name;
#endif
		if (!strcmp(str1, str2)) {

			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

void
output_line(struct process * proc, char *fmt, ...) {
	va_list args;

	if (current_pid) {
		fprintf(output, " <unfinished ...>\n");
	}
	current_pid=0;
	if (!fmt) {
		return;
	}
	begin_of_line(LT_TOF_NONE, proc);

	va_start(args, fmt);
	vfprintf(output, fmt, args);
	fprintf(output, "\n");
	va_end(args);
	current_column=0;
}

static void
tabto(int col) {
	if (current_column < col) {
		fprintf(output, "%*s", col-current_column, "");
	}
}

void
output_left(enum tof type, struct process * proc, char * function_name) {
	struct function * func;

	if (current_pid) {
		fprintf(output, " <unfinished ...>\n");
		current_pid=0;
		current_column=0;
	}
	current_pid = proc->pid;
	current_depth = proc->callstack_depth;
	proc->type_being_displayed = type;
	begin_of_line(type, proc);
#if HAVE_LIBIBERTY
	current_column += fprintf(output, "%s(", opt_C ? my_demangle(function_name): function_name);
#else
	current_column += fprintf(output, "%s(", function_name);
#endif

	func = name2func(function_name);
	if (!func) {
		int i;
		for(i=0; i<4; i++) {
			current_column += display_arg(type, proc, i, ARGTYPE_UNKNOWN);
			current_column += fprintf(output, ", ");
		}
		current_column += display_arg(type, proc, 4, ARGTYPE_UNKNOWN);
		return;
	} else {
		int i;
		for(i=0; i< func->num_params - func->params_right - 1; i++) {
			current_column += display_arg(type, proc, i, func->arg_types[i]);
			current_column += fprintf(output, ", ");
		}
		if (func->num_params>func->params_right) {
			current_column += display_arg(type, proc, i, func->arg_types[i]);
			if (func->params_right) {
				current_column += fprintf(output, ", ");
			}
		}
	}
}

void
output_right(enum tof type, struct process * proc, char * function_name) {
	struct function * func = name2func(function_name);

	if ((current_pid && current_pid!=proc->pid) ||
			current_depth != proc->callstack_depth) {
		fprintf(output, " <unfinished ...>\n");
		current_pid = 0;
	}
	if (current_pid != proc->pid) {
		begin_of_line(type, proc);
#if HAVE_LIBIBERTY
		current_column += fprintf(output, "<... %s resumed> ", opt_C ? my_demangle(function_name) : function_name);
#else
		current_column += fprintf(output, "<... %s resumed> ", function_name);
#endif
	}

	if (!func) {
		current_column += fprintf(output, ") ");
		tabto(opt_a);
		fprintf(output, "= ");
		display_arg(type, proc, -1, ARGTYPE_UNKNOWN);
		fprintf(output, "\n");
	} else {
		int i;
		for(i=func->num_params-func->params_right; i<func->num_params-1; i++) {
			current_column += display_arg(type, proc, i, func->arg_types[i]);
			current_column += fprintf(output, ", ");
		}
		if (func->params_right) {
			current_column += display_arg(type, proc, i, func->arg_types[i]);
		}
		current_column += fprintf(output, ") ");
			tabto(opt_a);
			fprintf(output, "= ");
		if (func->return_type == ARGTYPE_VOID) {
			fprintf(output, "<void>");
		} else {
			display_arg(type, proc, -1, func->return_type);
		}
		fprintf(output, "\n");
	}
	current_pid=0;
	current_column=0;
}
