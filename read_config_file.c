#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ltrace.h"
#include "read_config_file.h"
#include "options.h"
#include "output.h"

/*
 *	"void"		LT_PT_VOID
 *	"int"		LT_PT_INT
 *	"uint"		LT_PT_UINT
 *	"octal"		LT_PT_OCTAL
 *	"char"		LT_PT_CHAR
 *	"string"	LT_PT_STRING
 *	"format"	LT_PT_FORMAT
 *	"addr"		LT_PT_ADDR
 */

struct function * list_of_functions = NULL;

static struct list_of_pt_t {
	char * name;
	enum param_type pt;
} list_of_pt[] = {
	{ "void",	LT_PT_VOID },
	{ "int",	LT_PT_INT },
	{ "uint",	LT_PT_UINT },
	{ "octal",	LT_PT_OCTAL },
	{ "char",	LT_PT_CHAR },
	{ "addr",	LT_PT_ADDR },
	{ "file",	LT_PT_FILE },
	{ "format",	LT_PT_FORMAT },
	{ "string",	LT_PT_STRING },
	{ "string0",	LT_PT_STRING0 },
	{ "string1",	LT_PT_STRING1 },
	{ "string2",	LT_PT_STRING2 },
	{ "string3",	LT_PT_STRING3 },
	{ NULL,		LT_PT_UNKNOWN }		/* Must finish with NULL */
};

static enum param_type str2type(char ** str)
{
	struct list_of_pt_t * tmp = &list_of_pt[0];

	while(tmp->name) {
		if (!strncmp(*str, tmp->name, strlen(tmp->name))
			&& index(" ,)#", *(*str+strlen(tmp->name)))) {
				*str += strlen(tmp->name);
				return tmp->pt;
		}
		tmp++;
	}
	return LT_PT_UNKNOWN;
}

static void eat_spaces(char ** str)
{
	while(**str==' ') {
		(*str)++;
	}
}

static int line_no;
static char * filename;

struct function * process_line (char * buf) {
	struct function fun;
	struct function * fun_p;
	char * str = buf;
	char * tmp;
	int i;

	line_no++;
	if (opt_d>2) {
		output_line(0, "Reading line %d of `%s'", line_no, filename);
	}
	eat_spaces(&str);
	fun.return_type = str2type(&str);
	if (fun.return_type==LT_PT_UNKNOWN) {
		if (opt_d>2) {
			output_line(0, " Skipping line %d", line_no);
		}
		return NULL;
	}
	if (opt_d>3) {
		output_line(0, " return_type = %d", fun.return_type);
	}
	eat_spaces(&str);
	tmp = strpbrk(str, " (");
	if (!tmp) {
		output_line(0, "Syntax error in `%s', line %d", filename, line_no);
		return NULL;
	}
	*tmp = '\0';
	fun.name = strdup(str);
	str = tmp+1;
	if (opt_d>2) {
		output_line(0, " name = %s", fun.name);
	}
	fun.params_right = 0;
	for(i=0; i<MAX_ARGS; i++) {
		eat_spaces(&str);
		if (*str == ')') {
			break;
		}
		if (str[0]=='+') {
			fun.params_right++;
			str++;
		} else if (fun.params_right) {
			fun.params_right++;
		}
		fun.param_types[i] = str2type(&str);
		if (fun.return_type==LT_PT_UNKNOWN) {
			output_line(0, "Syntax error in `%s', line %d", filename, line_no);
			return NULL;
		}
		eat_spaces(&str);
		if (*str==',') {
			str++;
			continue;
		} else if (*str==')') {
			continue;
		} else {
			output_line(0, "Syntax error in `%s', line %d", filename, line_no);
			return NULL;
		}
	}
	fun.num_params = i;
	fun_p = malloc(sizeof(struct function));
	memcpy(fun_p, &fun, sizeof(struct function));
	return fun_p;
}

void read_config_file(char * file)
{
	FILE * stream;
	char buf[1024];

	filename = file;

	if (opt_d) {
		output_line(0, "Reading config file `%s'...", filename);
	}

	stream = fopen(filename, "r");
	if (!stream) {
		return;
	}
	line_no=0;
	while (fgets(buf, 1024, stream)) {
		struct function * tmp = process_line(buf);

		if (tmp) {
			if (opt_d > 1) {
				output_line(0, "New function: `%s'", tmp->name);
			}
			tmp->next = list_of_functions;
			list_of_functions = tmp;
		}
	}
}
