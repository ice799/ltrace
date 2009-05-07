#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"
#include "dict.h"

#ifdef USE_DEMANGLE
#include "demangle.h"
#endif

/* TODO FIXME XXX: include in ltrace.h: */
extern struct timeval current_time_spent;

struct dict *dict_opt_c = NULL;

static Process *current_proc = 0;
static int current_depth = 0;
static int current_column = 0;

static void
output_indent(Process *proc) {
	current_column +=
	    fprintf(options.output, "%*s", options.indent * proc->callstack_depth, "");
}

static void
begin_of_line(enum tof type, Process *proc) {
	current_column = 0;
	if (!proc) {
		return;
	}
	if (list_of_processes->next) {
		current_column += fprintf(options.output, "[pid %u] ", proc->pid);
	}
	if (opt_r) {
		struct timeval tv;
		struct timezone tz;
		static struct timeval old_tv = { 0, 0 };
		struct timeval diff;

		gettimeofday(&tv, &tz);

		if (old_tv.tv_sec == 0 && old_tv.tv_usec == 0) {
			old_tv.tv_sec = tv.tv_sec;
			old_tv.tv_usec = tv.tv_usec;
		}
		diff.tv_sec = tv.tv_sec - old_tv.tv_sec;
		if (tv.tv_usec >= old_tv.tv_usec) {
			diff.tv_usec = tv.tv_usec - old_tv.tv_usec;
		} else {
			diff.tv_sec--;
			diff.tv_usec = 1000000 + tv.tv_usec - old_tv.tv_usec;
		}
		old_tv.tv_sec = tv.tv_sec;
		old_tv.tv_usec = tv.tv_usec;
		current_column += fprintf(options.output, "%3lu.%06d ",
					  diff.tv_sec, (int)diff.tv_usec);
	}
	if (opt_t) {
		struct timeval tv;
		struct timezone tz;

		gettimeofday(&tv, &tz);
		if (opt_t > 2) {
			current_column += fprintf(options.output, "%lu.%06d ",
						  tv.tv_sec, (int)tv.tv_usec);
		} else if (opt_t > 1) {
			struct tm *tmp = localtime(&tv.tv_sec);
			current_column +=
			    fprintf(options.output, "%02d:%02d:%02d.%06d ",
				    tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
				    (int)tv.tv_usec);
		} else {
			struct tm *tmp = localtime(&tv.tv_sec);
			current_column += fprintf(options.output, "%02d:%02d:%02d ",
						  tmp->tm_hour, tmp->tm_min,
						  tmp->tm_sec);
		}
	}
	if (opt_i) {
		if (type == LT_TOF_FUNCTION || type == LT_TOF_FUNCTIONR) {
			current_column += fprintf(options.output, "[%p] ",
						  proc->return_addr);
		} else {
			current_column += fprintf(options.output, "[%p] ",
						  proc->instruction_pointer);
		}
	}
	if (options.indent > 0 && type != LT_TOF_NONE) {
		output_indent(proc);
	}
}

static Function *
name2func(char *name) {
	Function *tmp;
	const char *str1, *str2;

	tmp = list_of_functions;
	while (tmp) {
#ifdef USE_DEMANGLE
		str1 = options.demangle ? my_demangle(tmp->name) : tmp->name;
		str2 = options.demangle ? my_demangle(name) : name;
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
output_line(Process *proc, char *fmt, ...) {
	va_list args;

	if (options.summary) {
		return;
	}
	if (current_proc) {
		if (current_proc->callstack[current_depth].return_addr) {
			fprintf(options.output, " <unfinished ...>\n");
		} else {
			fprintf(options.output, " <no return ...>\n");
		}
	}
	current_proc = 0;
	if (!fmt) {
		return;
	}
	begin_of_line(LT_TOF_NONE, proc);

	va_start(args, fmt);
	vfprintf(options.output, fmt, args);
	fprintf(options.output, "\n");
	va_end(args);
	current_column = 0;
}

static void
tabto(int col) {
	if (current_column < col) {
		fprintf(options.output, "%*s", col - current_column, "");
	}
}

void
output_left(enum tof type, Process *proc, char *function_name) {
	Function *func;
	static arg_type_info *arg_unknown = NULL;
	if (arg_unknown == NULL)
	    arg_unknown = lookup_prototype(ARGTYPE_UNKNOWN);

	if (options.summary) {
		return;
	}
	if (current_proc) {
		fprintf(options.output, " <unfinished ...>\n");
		current_proc = 0;
		current_column = 0;
	}
	current_proc = proc;
	current_depth = proc->callstack_depth;
	proc->type_being_displayed = type;
	begin_of_line(type, proc);
#ifdef USE_DEMANGLE
	current_column +=
	    fprintf(options.output, "%s(",
		    options.demangle ? my_demangle(function_name) : function_name);
#else
	current_column += fprintf(options.output, "%s(", function_name);
#endif

	func = name2func(function_name);
	if (!func) {
		int i;
		for (i = 0; i < 4; i++) {
			current_column +=
			    display_arg(type, proc, i, arg_unknown);
			current_column += fprintf(options.output, ", ");
		}
		current_column += display_arg(type, proc, 4, arg_unknown);
		return;
	} else {
		int i;
		for (i = 0; i < func->num_params - func->params_right - 1; i++) {
			current_column +=
			    display_arg(type, proc, i, func->arg_info[i]);
			current_column += fprintf(options.output, ", ");
		}
		if (func->num_params > func->params_right) {
			current_column +=
			    display_arg(type, proc, i, func->arg_info[i]);
			if (func->params_right) {
				current_column += fprintf(options.output, ", ");
			}
		}
		if (func->params_right) {
			save_register_args(type, proc);
		}
	}
}

void
output_right(enum tof type, Process *proc, char *function_name) {
	Function *func = name2func(function_name);
	static arg_type_info *arg_unknown = NULL;
	if (arg_unknown == NULL)
	    arg_unknown = lookup_prototype(ARGTYPE_UNKNOWN);

	if (options.summary) {
		struct opt_c_struct *st;
		if (!dict_opt_c) {
			dict_opt_c =
			    dict_init(dict_key2hash_string,
				      dict_key_cmp_string);
		}
		st = dict_find_entry(dict_opt_c, function_name);
		if (!st) {
			char *na;
			st = malloc(sizeof(struct opt_c_struct));
			na = strdup(function_name);
			if (!st || !na) {
				perror("malloc()");
				exit(1);
			}
			st->count = 0;
			st->tv.tv_sec = st->tv.tv_usec = 0;
			dict_enter(dict_opt_c, na, st);
		}
		if (st->tv.tv_usec + current_time_spent.tv_usec > 1000000) {
			st->tv.tv_usec += current_time_spent.tv_usec - 1000000;
			st->tv.tv_sec++;
		} else {
			st->tv.tv_usec += current_time_spent.tv_usec;
		}
		st->count++;
		st->tv.tv_sec += current_time_spent.tv_sec;

//              fprintf(options.output, "%s <%lu.%06d>\n", function_name,
//                              current_time_spent.tv_sec, (int)current_time_spent.tv_usec);
		return;
	}
	if (current_proc && (current_proc != proc ||
			    current_depth != proc->callstack_depth)) {
		fprintf(options.output, " <unfinished ...>\n");
		current_proc = 0;
	}
	if (current_proc != proc) {
		begin_of_line(type, proc);
#ifdef USE_DEMANGLE
		current_column +=
		    fprintf(options.output, "<... %s resumed> ",
			    options.demangle ? my_demangle(function_name) : function_name);
#else
		current_column +=
		    fprintf(options.output, "<... %s resumed> ", function_name);
#endif
	}

	if (!func) {
		current_column += fprintf(options.output, ") ");
		tabto(options.align - 1);
		fprintf(options.output, "= ");
		display_arg(type, proc, -1, arg_unknown);
	} else {
		int i;
		for (i = func->num_params - func->params_right;
		     i < func->num_params - 1; i++) {
			current_column +=
			    display_arg(type, proc, i, func->arg_info[i]);
			current_column += fprintf(options.output, ", ");
		}
		if (func->params_right) {
			current_column +=
			    display_arg(type, proc, i, func->arg_info[i]);
		}
		current_column += fprintf(options.output, ") ");
		tabto(options.align - 1);
		fprintf(options.output, "= ");
		if (func->return_info->type == ARGTYPE_VOID) {
			fprintf(options.output, "<void>");
		} else {
			display_arg(type, proc, -1, func->return_info);
		}
	}
	if (opt_T) {
		fprintf(options.output, " <%lu.%06d>",
			current_time_spent.tv_sec,
			(int)current_time_spent.tv_usec);
	}
	fprintf(options.output, "\n");
	current_proc = 0;
	current_column = 0;
}
