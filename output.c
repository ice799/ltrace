#include <stdio.h>
#include <stdarg.h>

#include "ltrace.h"
#include "options.h"
#include "output.h"

static pid_t current_pid = 0;
static int current_column = 0;

static void begin_of_line(enum tof type, struct process * proc)
{
	current_column = 0;
	if (!proc) {
		return;
	}
	if (list_of_processes && list_of_processes->next) {
		current_column += fprintf(output, "[pid %d] ", proc->pid);
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
}

static struct function * name2func(char * name)
{
	struct function * tmp;

	tmp = list_of_functions;
	while(tmp) {
		if (!strcmp(tmp->name, name)) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

void output_line(struct process * proc, char *fmt, ...)
{
	va_list args;

	if (current_pid) {
		fprintf(output, " <unfinished ...>\n");
	}
	begin_of_line(LT_TOF_NONE, proc);

        va_start(args, fmt);
        vfprintf(output, fmt, args);
        fprintf(output, "\n");
        va_end(args);
	current_pid=0;
	current_column=0;
}

static void tabto(int col)
{
	if (current_column < col) {
		fprintf(output, "%*s", col-current_column, "");
	}
}

void output_left(enum tof type, struct process * proc, char * function_name)
{
	struct function * func;

	if (current_pid) {
#if 1						/* ugly hack :) */
		if (current_pid == proc->pid
			&& proc->type_being_displayed == LT_TOF_FUNCTION
			&& proc->type_being_displayed == type) {
				tabto(opt_a);
				fprintf(output, "= ???\n");
		} else
#endif
			fprintf(output, " <unfinished ...>\n");
		current_pid=0;
		current_column=0;
	}
	current_pid=proc->pid;
	proc->type_being_displayed = type;
	begin_of_line(type, proc);
	current_column += fprintf(output, "%s(", function_name);

	func = name2func(function_name);
	if (!func) {
		int i;
		for(i=0; i<4; i++) {
			current_column += display_arg(type, proc, i, LT_PT_UNKNOWN);
			current_column += fprintf(output, ", ");
		}
		current_column += display_arg(type, proc, 4, LT_PT_UNKNOWN);
		return;
	} else {
		int i;
		for(i=0; i< func->num_params - func->params_right - 1; i++) {
			current_column += display_arg(type, proc, i, func->param_types[i]);
			current_column += fprintf(output, ", ");
		}
		if (func->num_params>func->params_right) {
			current_column += display_arg(type, proc, i, func->param_types[i]);
			if (func->params_right) {
				current_column += fprintf(output, ", ");
			}
		}
		if (!func->params_right && func->return_type == LT_PT_VOID) {
			current_column += fprintf(output, ") ");
			tabto(opt_a);
			fprintf(output, "= <void>\n");
			current_pid = 0;
			current_column = 0;
		}
	}
}

void output_right(enum tof type, struct process * proc, char * function_name)
{
	struct function * func = name2func(function_name);

	if (func && func->params_right==0 && func->return_type == LT_PT_VOID) {
		return;
	}

	if (current_pid && current_pid!=proc->pid) {
		fprintf(output, " <unfinished ...>\n");
		begin_of_line(type, proc);
		current_column += fprintf(output, "<... %s resumed> ", function_name);
	} else if (!current_pid) {
		begin_of_line(type, proc);
		current_column += fprintf(output, "<... %s resumed> ", function_name);
	}

	if (!func) {
		current_column += fprintf(output, ") ");
		tabto(opt_a);
		fprintf(output, "= ");
		display_arg(type, proc, -1, LT_PT_UNKNOWN);
		fprintf(output, "\n");
	} else {
		int i;
		for(i=func->num_params-func->params_right; i<func->num_params-1; i++) {
			current_column += display_arg(type, proc, i, func->param_types[i]);
			current_column += fprintf(output, ", ");
		}
		if (func->params_right) {
			current_column += display_arg(type, proc, i, func->param_types[i]);
		}
		current_column += fprintf(output, ") ");
			tabto(opt_a);
			fprintf(output, "= ");
		if (func->return_type == LT_PT_VOID) {
			fprintf(output, "<void>");
		} else {
			display_arg(type, proc, -1, func->return_type);
		}
		fprintf(output, "\n");
	}
	current_pid=0;
	current_column=0;
}
