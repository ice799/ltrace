#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ltrace.h"
#include "read_config_file.h"
#include "output.h"
#include "debug.h"

/*
 *	"void"		ARGTYPE_VOID
 *	"int"		ARGTYPE_INT
 *	"uint"		ARGTYPE_UINT
 *	"long"		ARGTYPE_LONG
 *	"ulong"		ARGTYPE_ULONG
 *	"octal"		ARGTYPE_OCTAL
 *	"char"		ARGTYPE_CHAR
 *	"string"	ARGTYPE_STRING
 *	"format"	ARGTYPE_FORMAT
 *	"addr"		ARGTYPE_ADDR
 */

struct function * list_of_functions = NULL;

static struct list_of_pt_t {
	char * name;
	enum arg_type pt;
} list_of_pt[] = {
	{ "void",   ARGTYPE_VOID },
	{ "int",    ARGTYPE_INT },
	{ "uint",   ARGTYPE_UINT },
	{ "long",   ARGTYPE_LONG },
	{ "ulong",  ARGTYPE_ULONG },
	{ "octal",  ARGTYPE_OCTAL },
	{ "char",   ARGTYPE_CHAR },
	{ "addr",   ARGTYPE_ADDR },
	{ "file",   ARGTYPE_FILE },
	{ "format", ARGTYPE_FORMAT },
	{ "string", ARGTYPE_STRING },
	{ NULL,     ARGTYPE_UNKNOWN }		/* Must finish with NULL */
};

static struct complete_arg_type
str2type(char ** str) {
	struct list_of_pt_t * tmp = &list_of_pt[0];
	struct complete_arg_type pt = {0,0};

 	if (!strncmp(*str, "string", 6)
	    && isdigit((unsigned char)(*str)[6]))
	{
		pt.at = ARGTYPE_STRINGN;
		pt.argno = atoi(*str + 6);
		*str += strspn(*str, "string0123456789");
		return pt;
 	}

	while(tmp->name) {
		if (!strncmp(*str, tmp->name, strlen(tmp->name))
			&& index(" ,)#", *(*str+strlen(tmp->name)))) {
				*str += strlen(tmp->name);
				pt.at = tmp->pt;
				return pt;
		}
		tmp++;
	}
	pt.at = ARGTYPE_UNKNOWN;
	return pt;
}

static void
eat_spaces(char ** str) {
	while(**str==' ') {
		(*str)++;
	}
}

/*
  Returns position in string at the left parenthesis which starts the
  function's argument signature. Returns NULL on error.
*/
static char *
start_of_arg_sig(char * str) {
	char * pos;
	int stacked = 0;

	if (!strlen(str)) return NULL;

	pos = &str[strlen(str)];
	do {
		pos--;
		if (pos < str) return NULL;
		while ((pos > str) && (*pos != ')') && (*pos != '(')) pos--;

		if (*pos == ')') stacked++;
		else if (*pos == '(') stacked--;
		else return NULL;

	} while (stacked > 0);

	return (stacked == 0) ? pos : NULL;
}

static int line_no;
static char * filename;

static struct function *
process_line (char * buf) {
	struct function fun;
	struct function * fun_p;
	char * str = buf;
	char * tmp;
	int i;

	line_no++;
	debug(3, "Reading line %d of `%s'", line_no, filename);
	eat_spaces(&str);
	fun.return_type = str2type(&str);
	if (fun.return_type.at==ARGTYPE_UNKNOWN) {
		debug(3, " Skipping line %d", line_no);
		return NULL;
	}
	debug(4, " return_type = %d", fun.return_type);
	eat_spaces(&str);
	tmp = start_of_arg_sig(str);
	if (!tmp) {
		output_line(0, "Syntax error in `%s', line %d", filename, line_no);
		return NULL;
	}
	*tmp = '\0';
	fun.name = strdup(str);
	str = tmp+1;
	debug(3, " name = %s", fun.name);
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
		fun.arg_types[i] = str2type(&str);
		if (fun.return_type.at==ARGTYPE_UNKNOWN) {
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

void
read_config_file(char * file) {
	FILE * stream;
	char buf[1024];

	filename = file;

	debug(1, "Reading config file `%s'...", filename);

	stream = fopen(filename, "r");
	if (!stream) {
		return;
	}
	line_no=0;
	while (fgets(buf, 1024, stream)) {
		struct function * tmp = process_line(buf);

		if (tmp) {
			debug(2, "New function: `%s'", tmp->name);
			tmp->next = list_of_functions;
			list_of_functions = tmp;
		}
	}
	fclose(stream);
}
