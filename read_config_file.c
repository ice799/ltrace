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

struct function *list_of_functions = NULL;

static struct list_of_pt_t {
	char *name;
	enum arg_type pt;
} list_of_pt[] = {
	{
	"void", ARGTYPE_VOID}, {
	"int", ARGTYPE_INT}, {
	"uint", ARGTYPE_UINT}, {
	"long", ARGTYPE_LONG}, {
	"ulong", ARGTYPE_ULONG}, {
	"octal", ARGTYPE_OCTAL}, {
	"char", ARGTYPE_CHAR}, {
	"addr", ARGTYPE_ADDR}, {
	"file", ARGTYPE_FILE}, {
	"format", ARGTYPE_FORMAT}, {
	"string", ARGTYPE_STRING}, {
	"ignore", ARGTYPE_IGNORE}, {
	NULL, ARGTYPE_UNKNOWN}	/* Must finish with NULL */
};

static arg_type_info arg_type_singletons[] = {
	{ ARGTYPE_VOID },
	{ ARGTYPE_INT },
	{ ARGTYPE_UINT },
	{ ARGTYPE_LONG },
	{ ARGTYPE_ULONG },
	{ ARGTYPE_OCTAL },
	{ ARGTYPE_CHAR },
	{ ARGTYPE_ADDR },
	{ ARGTYPE_FILE },
	{ ARGTYPE_FORMAT },
	{ ARGTYPE_STRING },
	{ ARGTYPE_STRING_N },
	{ ARGTYPE_IGNORE },
	{ ARGTYPE_UNKNOWN }
};

arg_type_info *lookup_singleton(enum arg_type at)
{
	if (at >= 0 && at <= ARGTYPE_COUNT)
		return &arg_type_singletons[at];
	else
		return &arg_type_singletons[ARGTYPE_COUNT]; /* UNKNOWN */
}

static arg_type_info *str2type(char **str)
{
	struct list_of_pt_t *tmp = &list_of_pt[0];

	while (tmp->name) {
		if (!strncmp(*str, tmp->name, strlen(tmp->name))
		    && index(" ,()#;012345[", *(*str + strlen(tmp->name)))) {
			*str += strlen(tmp->name);
			return lookup_singleton(tmp->pt);
		}
		tmp++;
	}
	return lookup_singleton(ARGTYPE_UNKNOWN);
}

static void eat_spaces(char **str)
{
	while (**str == ' ') {
		(*str)++;
	}
}

/*
  Returns position in string at the left parenthesis which starts the
  function's argument signature. Returns NULL on error.
*/
static char *start_of_arg_sig(char *str)
{
	char *pos;
	int stacked = 0;

	if (!strlen(str))
		return NULL;

	pos = &str[strlen(str)];
	do {
		pos--;
		if (pos < str)
			return NULL;
		while ((pos > str) && (*pos != ')') && (*pos != '('))
			pos--;

		if (*pos == ')')
			stacked++;
		else if (*pos == '(')
			stacked--;
		else
			return NULL;

	} while (stacked > 0);

	return (stacked == 0) ? pos : NULL;
}

/*
  Decide whether a type needs any additional parameters.
  For now, we do not parse any nontrivial argument types.
*/
static int simple_type(enum arg_type at)
{
	return 1;
}

static int line_no;
static char *filename;

static int parse_int(char **str)
{
    char *end;
    long n = strtol(*str, &end, 0);
    if (end == *str) {
	output_line(0, "Syntax error in `%s', line %d: Bad number",
		    filename, line_no);
	return 0;
    }

    *str = end;
    return n;
}

/*
 * Input:
 *  argN   : The value of argument #N, counting from 1 (arg0 = retval)
 *  eltN   : The value of element #N of the containing structure
 *  retval : The return value
 *  0      : Error
 *  N      : The numeric value N, if N > 0
 *
 * Output:
 * > 0   actual numeric value
 * = 0   return value
 * < 0   (arg -n), counting from one
 */
static int parse_argnum(char **str)
{
    int multiplier = 1;
    int n = 0;

    if (strncmp(*str, "arg", 3) == 0) {
	(*str) += 3;
	multiplier = -1;
    } else if (strncmp(*str, "retval", 6) == 0) {
	(*str) += 6;
	return 0;
    }

    n = parse_int(str);

    return n * multiplier;
}

static arg_type_info *parse_type(char **str)
{
	arg_type_info *simple;
	arg_type_info *info;

	simple = str2type(str);
	if (simple->type == ARGTYPE_UNKNOWN) {
		return simple;		// UNKNOWN
	}

	if (simple_type(simple->type) && simple->type != ARGTYPE_STRING)
		return simple;

	info = malloc(sizeof(*info));
	info->type = simple->type;

	/* Code to parse parameterized types will go into the following
	   switch statement. */

	switch (info->type) {

	case ARGTYPE_STRING:
	    if (!isdigit(**str) && **str != '[') {
		/* Oops, was just a simple string after all */
		free(info);
		return simple;
	    }

	    info->type = ARGTYPE_STRING_N;

	    /* Backwards compatibility for string0, string1, ... */
	    if (isdigit(**str)) {
		info->u.string_n_info.size_spec = -parse_int(str);
		return info;
	    }

	    (*str)++;		// Skip past opening [
	    eat_spaces(str);
	    info->u.string_n_info.size_spec = parse_argnum(str);
	    eat_spaces(str);
	    (*str)++;		// Skip past closing ]
	    return info;

	default:
		output_line(0, "Syntax error in `%s', line %d: Unknown type encountered",
			    filename, line_no);
		free(info);
		return NULL;
	}
}

static struct function *process_line(char *buf)
{
	struct function fun;
	struct function *fun_p;
	char *str = buf;
	char *tmp;
	int i;

	line_no++;
	debug(3, "Reading line %d of `%s'", line_no, filename);
	eat_spaces(&str);
	fun.return_info = parse_type(&str);
	if (fun.return_info == NULL)
        	return NULL;
	if (fun.return_info->type == ARGTYPE_UNKNOWN) {
		debug(3, " Skipping line %d", line_no);
		return NULL;
	}
	debug(4, " return_type = %d", fun.return_info->type);
	eat_spaces(&str);
	tmp = start_of_arg_sig(str);
	if (!tmp) {
		output_line(0, "Syntax error in `%s', line %d", filename,
			    line_no);
		return NULL;
	}
	*tmp = '\0';
	fun.name = strdup(str);
	str = tmp + 1;
	debug(3, " name = %s", fun.name);
	fun.params_right = 0;
	for (i = 0; i < MAX_ARGS; i++) {
		eat_spaces(&str);
		if (*str == ')') {
			break;
		}
		if (str[0] == '+') {
			fun.params_right++;
			str++;
		} else if (fun.params_right) {
			fun.params_right++;
		}
		fun.arg_info[i] = parse_type(&str);
		if (fun.arg_info[i] == NULL) {
			output_line(0, "Syntax error in `%s', line %d"
                                    ": unknown argument type",
				    filename, line_no);
			return NULL;
		}
		eat_spaces(&str);
		if (*str == ',') {
			str++;
			continue;
		} else if (*str == ')') {
			continue;
		} else {
			if (str[strlen(str) - 1] == '\n')
				str[strlen(str) - 1] = '\0';
			output_line(0, "Syntax error in `%s', line %d at ...\"%s\"",
				    filename, line_no, str);
			return NULL;
		}
	}
	fun.num_params = i;
	fun_p = malloc(sizeof(struct function));
	memcpy(fun_p, &fun, sizeof(struct function));
	return fun_p;
}

void read_config_file(char *file)
{
	FILE *stream;
	char buf[1024];

	filename = file;

	debug(1, "Reading config file `%s'...", filename);

	stream = fopen(filename, "r");
	if (!stream) {
		return;
	}
	line_no = 0;
	while (fgets(buf, 1024, stream)) {
		struct function *tmp = process_line(buf);

		if (tmp) {
			debug(2, "New function: `%s'", tmp->name);
			tmp->next = list_of_functions;
			list_of_functions = tmp;
		}
	}
	fclose(stream);
}
