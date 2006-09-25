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

static int line_no;
static char *filename;
static int error_count = 0;

static arg_type_info *parse_type(char **str);

struct function *list_of_functions = NULL;

/* Map of strings to type names. These do not need to be in any
 * particular order */
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
	"short", ARGTYPE_SHORT}, {
	"ushort", ARGTYPE_USHORT}, {
	"float", ARGTYPE_FLOAT}, {
	"double", ARGTYPE_DOUBLE}, {
	"addr", ARGTYPE_ADDR}, {
	"file", ARGTYPE_FILE}, {
	"format", ARGTYPE_FORMAT}, {
	"string", ARGTYPE_STRING}, {
	"array", ARGTYPE_ARRAY}, {
	"struct", ARGTYPE_STRUCT}, {
	"enum", ARGTYPE_ENUM}, {
	"ignore", ARGTYPE_IGNORE}, {
	NULL, ARGTYPE_UNKNOWN}	/* Must finish with NULL */
};

/* Array of prototype objects for each of the types. The order in this
 * array must exactly match the list of enumerated values in
 * ltrace.h */
static arg_type_info arg_type_prototypes[] = {
	{ ARGTYPE_VOID },
	{ ARGTYPE_INT },
	{ ARGTYPE_UINT },
	{ ARGTYPE_LONG },
	{ ARGTYPE_ULONG },
	{ ARGTYPE_OCTAL },
	{ ARGTYPE_CHAR },
	{ ARGTYPE_SHORT },
	{ ARGTYPE_USHORT },
	{ ARGTYPE_FLOAT },
	{ ARGTYPE_DOUBLE },
	{ ARGTYPE_ADDR },
	{ ARGTYPE_FILE },
	{ ARGTYPE_FORMAT },
	{ ARGTYPE_STRING },
	{ ARGTYPE_STRING_N },
	{ ARGTYPE_ARRAY },
	{ ARGTYPE_ENUM },
	{ ARGTYPE_STRUCT },
	{ ARGTYPE_IGNORE },
	{ ARGTYPE_POINTER },
	{ ARGTYPE_UNKNOWN }
};

arg_type_info *lookup_prototype(enum arg_type at)
{
	if (at >= 0 && at <= ARGTYPE_COUNT)
		return &arg_type_prototypes[at];
	else
		return &arg_type_prototypes[ARGTYPE_COUNT]; /* UNKNOWN */
}

static arg_type_info *str2type(char **str)
{
	struct list_of_pt_t *tmp = &list_of_pt[0];

	while (tmp->name) {
		if (!strncmp(*str, tmp->name, strlen(tmp->name))
		    && index(" ,()#*;012345[", *(*str + strlen(tmp->name)))) {
			*str += strlen(tmp->name);
			return lookup_prototype(tmp->pt);
		}
		tmp++;
	}
	return lookup_prototype(ARGTYPE_UNKNOWN);
}

static void eat_spaces(char **str)
{
	while (**str == ' ') {
		(*str)++;
	}
}

static char *xstrndup(char *str, size_t len)
{
    char *ret = (char *) malloc(len + 1);
    strncpy(ret, str, len);
    ret[len] = 0;
    return ret;
}

static char *parse_ident(char **str)
{
    char *ident = *str;

    if (!isalnum(**str) && **str != '_') {
	output_line(0, "Syntax error in `%s', line %d: Bad identifier",
		    filename, line_no);
	error_count++;
	return NULL;
    }

    while (**str && (isalnum(**str) || **str == '_')) {
	++(*str);
    }

    return xstrndup(ident, *str - ident);
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

static int parse_int(char **str)
{
    char *end;
    long n = strtol(*str, &end, 0);
    if (end == *str) {
	output_line(0, "Syntax error in `%s', line %d: Bad number (%s)",
		    filename, line_no, *str);
	error_count++;
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
    } else if (strncmp(*str, "elt", 3) == 0) {
	(*str) += 3;
	multiplier = -1;
    } else if (strncmp(*str, "retval", 6) == 0) {
	(*str) += 6;
	return 0;
    }

    n = parse_int(str);

    return n * multiplier;
}

struct typedef_node_t {
    char *name;
    arg_type_info *info;
    struct typedef_node_t *next;
} *typedefs = NULL;

static arg_type_info *lookup_typedef(char **str)
{
    struct typedef_node_t *node;
    char *end = *str;
    while (*end && (isalnum(*end) || *end == '_'))
	++end;
    if (end == *str)
	return NULL;

    for (node = typedefs; node != NULL; node = node->next) {
	if (strncmp(*str, node->name, end - *str) == 0) {
	    (*str) += strlen(node->name);
	    return node->info;
	}
    }

    return NULL;
}

static void parse_typedef(char **str)
{
    char *name;
    arg_type_info *info;
    struct typedef_node_t *binding;

    (*str) += strlen("typedef");
    eat_spaces(str);

    // Grab out the name of the type
    name = parse_ident(str);

    // Skip = sign
    eat_spaces(str);
    if (**str != '=') {
	output_line(0,
		    "Syntax error in `%s', line %d: expected '=', got '%c'",
		    filename, line_no, **str);
	error_count++;
	return;
    }
    (*str)++;
    eat_spaces(str);

    // Parse the type
    info = parse_type(str);

    // Insert onto beginning of linked list
    binding = malloc(sizeof(*binding));
    binding->name = name;
    binding->info = info;
    binding->next = typedefs;
    typedefs = binding;
}

static size_t arg_sizeof(arg_type_info * arg)
{
    if (arg->type == ARGTYPE_CHAR) {
	return sizeof(char);
    } else if (arg->type == ARGTYPE_SHORT || arg->type == ARGTYPE_USHORT) {
	return sizeof(short);
    } else if (arg->type == ARGTYPE_FLOAT) {
	return sizeof(float);
    } else if (arg->type == ARGTYPE_DOUBLE) {
	return sizeof(double);
    } else if (arg->type == ARGTYPE_ENUM) {
	return sizeof(int);
    } else if (arg->type == ARGTYPE_STRUCT) {
	return arg->u.struct_info.size;
    } else if (arg->type == ARGTYPE_POINTER) {
	return sizeof(void*);
    } else if (arg->type == ARGTYPE_ARRAY) {
	if (arg->u.array_info.len_spec > 0)
	    return arg->u.array_info.len_spec * arg->u.array_info.elt_size;
	else
	    return sizeof(void *);
    } else {
	return sizeof(int);
    }
}

#undef alignof
#define alignof(field,st) ((size_t) ((char*) &st.field - (char*) &st))
static size_t arg_align(arg_type_info * arg)
{
    struct { char c; char C; } cC;
    struct { char c; short s; } cs;
    struct { char c; int i; } ci;
    struct { char c; long l; } cl;
    struct { char c; void* p; } cp;
    struct { char c; float f; } cf;
    struct { char c; double d; } cd;

    static size_t char_alignment = alignof(C, cC);
    static size_t short_alignment = alignof(s, cs);
    static size_t int_alignment = alignof(i, ci);
    static size_t long_alignment = alignof(l, cl);
    static size_t ptr_alignment = alignof(p, cp);
    static size_t float_alignment = alignof(f, cf);
    static size_t double_alignment = alignof(d, cd);

    switch (arg->type) {
    case ARGTYPE_LONG:
    case ARGTYPE_ULONG:
	return long_alignment;
    case ARGTYPE_CHAR:
	return char_alignment;
    case ARGTYPE_SHORT:
    case ARGTYPE_USHORT:
	return short_alignment;
    case ARGTYPE_FLOAT:
	return float_alignment;
    case ARGTYPE_DOUBLE:
	return double_alignment;
    case ARGTYPE_ADDR:
    case ARGTYPE_FILE:
    case ARGTYPE_FORMAT:
    case ARGTYPE_STRING:
    case ARGTYPE_STRING_N:
    case ARGTYPE_POINTER:
	return ptr_alignment;

    case ARGTYPE_ARRAY:
	return arg_align(&arg->u.array_info.elt_type[0]);

    case ARGTYPE_STRUCT:
	return arg_align(arg->u.struct_info.fields[0]);
	
    default:
	return int_alignment;
    }
}

static size_t align_skip(size_t alignment, size_t offset)
{
    if (offset % alignment)
	return alignment - (offset % alignment);
    else
	return 0;
}

/* I'm sure this isn't completely correct, but just try to get most of
 * them right for now. */
static void align_struct(arg_type_info* info)
{
    size_t offset;
    int i;

    if (info->u.struct_info.size != 0)
	return;			// Already done

    // Compute internal padding due to alignment constraints for
    // various types.
    offset = 0;
    for (i = 0; info->u.struct_info.fields[i] != NULL; i++) {
	arg_type_info *field = info->u.struct_info.fields[i];
	offset += align_skip(arg_align(field), offset);
	info->u.struct_info.offset[i] = offset;
	offset += arg_sizeof(field);
    }

    info->u.struct_info.size = offset;
}

static arg_type_info *parse_nonpointer_type(char **str)
{
	arg_type_info *simple;
	arg_type_info *info;

	if (strncmp(*str, "typedef", 7) == 0) {
	    parse_typedef(str);
	    return lookup_prototype(ARGTYPE_UNKNOWN);
	}

	simple = str2type(str);
	if (simple->type == ARGTYPE_UNKNOWN) {
	    info = lookup_typedef(str);
	    if (info)
		return info;
	    else
		return simple;		// UNKNOWN
	}

	info = malloc(sizeof(*info));
	info->type = simple->type;

	/* Code to parse parameterized types will go into the following
	   switch statement. */

	switch (info->type) {

	/* Syntax: array ( type, N|argN ) */
	case ARGTYPE_ARRAY:
	    (*str)++;		// Get past open paren
	    eat_spaces(str);
	    if ((info->u.array_info.elt_type = parse_type(str)) == NULL)
		return NULL;
	    info->u.array_info.elt_size =
		arg_sizeof(info->u.array_info.elt_type);
	    (*str)++;		// Get past comma
	    eat_spaces(str);
	    info->u.array_info.len_spec = parse_argnum(str);
	    (*str)++;		// Get past close paren
	    return info;

	/* Syntax: enum ( keyname=value,keyname=value,... ) */
	case ARGTYPE_ENUM:{
	    struct enum_opt {
		char *key;
		int value;
		struct enum_opt *next;
	    };
	    struct enum_opt *list = NULL;
	    struct enum_opt *p;
	    int entries = 0;
	    int ii;

	    eat_spaces(str);
	    (*str)++;		// Get past open paren
	    eat_spaces(str);

	    while (**str && **str != ')') {
		p = (struct enum_opt *) malloc(sizeof(*p));
		eat_spaces(str);
		p->key = parse_ident(str);
		if (error_count) {
		    free(p);
		    return NULL;
		}
		eat_spaces(str);
		if (**str != '=') {
		    free(p->key);
		    free(p);
		    output_line(0,
				"Syntax error in `%s', line %d: expected '=', got '%c'",
				filename, line_no, **str);
		    error_count++;
		    return NULL;
		}
		++(*str);
		eat_spaces(str);
		p->value = parse_int(str);
		p->next = list;
		list = p;
		++entries;

		// Skip comma
		eat_spaces(str);
		if (**str == ',') {
		    (*str)++;
		    eat_spaces(str);
		}
	    }

	    info->u.enum_info.entries = entries;
	    info->u.enum_info.keys =
		(char **) malloc(entries * sizeof(char *));
	    info->u.enum_info.values =
		(int *) malloc(entries * sizeof(int));
	    for (ii = 0, p = NULL; list; ++ii, list = list->next) {
		if (p)
		    free(p);
		info->u.enum_info.keys[ii] = list->key;
		info->u.enum_info.values[ii] = list->value;
		p = list;
	    }
	    if (p)
		free(p);

	    return info;
	}

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

	// Syntax: struct ( type,type,type,... )
    case ARGTYPE_STRUCT:{
	    int field_num = 0;
	    (*str)++;		// Get past open paren
	    info->u.struct_info.fields =
		malloc((MAX_ARGS + 1) * sizeof(void *));
	    info->u.struct_info.offset =
		malloc((MAX_ARGS + 1) * sizeof(size_t));
	    info->u.struct_info.size = 0;
	    eat_spaces(str); // Empty arg list with whitespace inside
	    while (**str && **str != ')') {
		if (field_num == MAX_ARGS) {
		    output_line(0,
				"Error in `%s', line %d: Too many structure elements",
				filename, line_no);
		    error_count++;
		    return NULL;
		}
		eat_spaces(str);
		if (field_num != 0) {
		    (*str)++;	// Get past comma
		    eat_spaces(str);
		}
		if ((info->u.struct_info.fields[field_num++] =
		     parse_type(str)) == NULL)
		    return NULL;

		// Must trim trailing spaces so the check for
		// the closing paren is simple
		eat_spaces(str);
	    }
	    (*str)++;		// Get past closing paren
	    info->u.struct_info.fields[field_num] = NULL;
	    align_struct(info);
	    return info;
	}

	default:
		if (info->type == ARGTYPE_UNKNOWN) {
			output_line(0, "Syntax error in `%s', line %d: "
				    "Unknown type encountered",
				    filename, line_no);
			free(info);
			error_count++;
			return NULL;
		} else {
			return info;
		}
	}
}

static arg_type_info *parse_type(char **str)
{
	arg_type_info *info = parse_nonpointer_type(str);
	while (**str == '*') {
		arg_type_info *outer = malloc(sizeof(*info));
		outer->type = ARGTYPE_POINTER;
		outer->u.ptr_info.info = info;
		(*str)++;
		info = outer;
	}
	return info;
}

static struct function *process_line(char *buf)
{
	struct function fun;
	struct function *fun_p;
	char *str = buf;
	char *tmp;
	int i;
	int float_num = 0;

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
	fun.return_info->arg_num = -1;
	debug(4, " return_type = %d", fun.return_info->type);
	eat_spaces(&str);
	tmp = start_of_arg_sig(str);
	if (!tmp) {
		output_line(0, "Syntax error in `%s', line %d", filename,
			    line_no);
		error_count++;
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
			error_count++;
			return NULL;
		}
		if (fun.arg_info[i]->type == ARGTYPE_FLOAT)
			fun.arg_info[i]->u.float_info.float_index = float_num++;
		else if (fun.arg_info[i]->type == ARGTYPE_DOUBLE)
			fun.arg_info[i]->u.double_info.float_index = float_num++;
		fun.arg_info[i]->arg_num = i;
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
			error_count++;
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
		struct function *tmp;

		error_count = 0;
		tmp = process_line(buf);

		if (tmp) {
			debug(2, "New function: `%s'", tmp->name);
			tmp->next = list_of_functions;
			list_of_functions = tmp;
		}
	}
	fclose(stream);
}
