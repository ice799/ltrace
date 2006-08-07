#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ltrace.h"
#include "options.h"

static int display_char(int what);
static int display_string(enum tof type, struct process *proc,
			  int arg_num, arg_type_info *info,
			  size_t maxlen);
static int display_unknown(enum tof type, struct process *proc, int arg_num);
static int display_format(enum tof type, struct process *proc, int arg_num);

static int string_maxlength = INT_MAX;

static long get_length(enum tof type, struct process *proc, int len_spec)
{
    if (len_spec > 0)
	return len_spec;
    return gimme_arg(type, proc, -len_spec - 1);
}

int
display_arg(enum tof type, struct process *proc,
	    int arg_num, arg_type_info *info)
{
	int tmp;
	long arg;

	switch (info->type) {
	case ARGTYPE_VOID:
		return 0;
	case ARGTYPE_INT:
		return fprintf(output, "%d",
			       (int)gimme_arg(type, proc, arg_num));
	case ARGTYPE_UINT:
		return fprintf(output, "%u",
			       (unsigned)gimme_arg(type, proc, arg_num));
	case ARGTYPE_LONG:
		if (proc->mask_32bit)
			return fprintf(output, "%d",
				       (int)gimme_arg(type, proc, arg_num));
		return fprintf(output, "%ld", gimme_arg(type, proc, arg_num));
	case ARGTYPE_ULONG:
		if (proc->mask_32bit)
			return fprintf(output, "%u",
				       (unsigned)gimme_arg(type, proc,
							   arg_num));
		return fprintf(output, "%lu",
			       (unsigned long)gimme_arg(type, proc, arg_num));
	case ARGTYPE_OCTAL:
		return fprintf(output, "0%o",
			       (unsigned)gimme_arg(type, proc, arg_num));
	case ARGTYPE_CHAR:
		tmp = fprintf(output, "'");
		tmp += display_char((int)gimme_arg(type, proc, arg_num));
		tmp += fprintf(output, "'");
		return tmp;
	case ARGTYPE_ADDR:
		arg = gimme_arg(type, proc, arg_num);
		if (!arg) {
			return fprintf(output, "NULL");
		} else {
			return fprintf(output, "%p", (void *)arg);
		}
	case ARGTYPE_FORMAT:
		return display_format(type, proc, arg_num);
	case ARGTYPE_STRING:
		return display_string(type, proc, arg_num, info,
				      string_maxlength);
	case ARGTYPE_STRING_N:
		return display_string(type, proc, arg_num, info,
				      get_length(type, proc,
						 info->u.string_n_info.size_spec));
	case ARGTYPE_UNKNOWN:
	default:
		return display_unknown(type, proc, arg_num);
	}
	return fprintf(output, "?");
}

static int display_char(int what)
{
	switch (what) {
	case -1:
		return fprintf(output, "EOF");
	case '\r':
		return fprintf(output, "\\r");
	case '\n':
		return fprintf(output, "\\n");
	case '\t':
		return fprintf(output, "\\t");
	case '\b':
		return fprintf(output, "\\b");
	case '\\':
		return fprintf(output, "\\\\");
	default:
		if ((what < 32) || (what > 126)) {
			return fprintf(output, "\\%03o", (unsigned char)what);
		} else {
			return fprintf(output, "%c", what);
		}
	}
}

#define MIN(a,b) (((a)<(b)) ? (a) : (b))

static int display_string(enum tof type, struct process *proc,
			  int arg_num, arg_type_info *info, size_t maxlength)
{
	void *addr;
	unsigned char *str1;
	int i;
	int len = 0;

	addr = (void *)gimme_arg(type, proc, arg_num);
	if (!addr) {
		return fprintf(output, "NULL");
	}

	str1 = malloc(MIN(opt_s, maxlength) + 3);
	if (!str1) {
		return fprintf(output, "???");
	}
	umovestr(proc, addr, MIN(opt_s, maxlength) + 1, str1);
	len = fprintf(output, "\"");
	for (i = 0; i < MIN(opt_s, maxlength); i++) {
		if (str1[i]) {
			len += display_char(str1[i]);
		} else {
			break;
		}
	}
	len += fprintf(output, "\"");
	if (str1[i] && (opt_s <= maxlength)) {
		len += fprintf(output, "...");
	}
	free(str1);
	return len;
}

static int display_unknown(enum tof type, struct process *proc, int arg_num)
{
	long tmp;

	tmp = gimme_arg(type, proc, arg_num);

	if (proc->mask_32bit) {
		if ((int)tmp < 1000000 && (int)tmp > -1000000)
			return fprintf(output, "%d", (int)tmp);
		else
			return fprintf(output, "%p", (void *)tmp);
	} else if (tmp < 1000000 && tmp > -1000000) {
		return fprintf(output, "%ld", tmp);
	} else {
		return fprintf(output, "%p", (void *)tmp);
	}
}

static int display_format(enum tof type, struct process *proc, int arg_num)
{
	void *addr;
	unsigned char *str1;
	int i;
	int len = 0;

	addr = (void *)gimme_arg(type, proc, arg_num);
	if (!addr) {
		return fprintf(output, "NULL");
	}

	str1 = malloc(MIN(opt_s, string_maxlength) + 3);
	if (!str1) {
		return fprintf(output, "???");
	}
	umovestr(proc, addr, MIN(opt_s, string_maxlength) + 1, str1);
	len = fprintf(output, "\"");
	for (i = 0; len < MIN(opt_s, string_maxlength) + 1; i++) {
		if (str1[i]) {
			len += display_char(str1[i]);
		} else {
			break;
		}
	}
	len += fprintf(output, "\"");
	if (str1[i] && (opt_s <= string_maxlength)) {
		len += fprintf(output, "...");
	}
	for (i = 0; str1[i]; i++) {
		if (str1[i] == '%') {
			int is_long = 0;
			while (1) {
				unsigned char c = str1[++i];
				if (c == '%') {
					break;
				} else if (!c) {
					break;
				} else if (strchr("lzZtj", c)) {
					is_long++;
					if (c == 'j')
						is_long++;
					if (is_long > 1
					    && (sizeof(long) < sizeof(long long)
						|| proc->mask_32bit)) {
						len += fprintf(output, ", ...");
						str1[i + 1] = '\0';
						break;
					}
				} else if (c == 'd' || c == 'i') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", %d",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", %ld",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == 'u') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", %u",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", %lu",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == 'o') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", 0%o",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", 0%lo",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == 'x' || c == 'X') {
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(output, ", %#x",
							    (int)gimme_arg(type,
									   proc,
									   ++arg_num));
					else
						len +=
						    fprintf(output, ", %#lx",
							    gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (strchr("eEfFgGaACS", c)
					   || (is_long
					       && (c == 'c' || c == 's'))) {
					len += fprintf(output, ", ...");
					str1[i + 1] = '\0';
					break;
				} else if (c == 'c') {
					len += fprintf(output, ", '");
					len +=
					    display_char((int)
							 gimme_arg(type, proc,
								   ++arg_num));
					len += fprintf(output, "'");
					break;
				} else if (c == 's') {
					arg_type_info *info =
					    lookup_singleton(ARGTYPE_STRING);
					len += fprintf(output, ", ");
					len +=
					    display_string(type, proc,
							   ++arg_num, info,
							   string_maxlength);
					break;
				} else if (c == 'p' || c == 'n') {
					len +=
					    fprintf(output, ", %p",
						    (void *)gimme_arg(type,
								      proc,
								      ++arg_num));
					break;
				} else if (c == '*') {
					len +=
					    fprintf(output, ", %d",
						    (int)gimme_arg(type, proc,
								   ++arg_num));
				}
			}
		}
	}
	free(str1);
	return len;
}
