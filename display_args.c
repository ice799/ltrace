#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "common.h"

static int display_char(int what);
static int display_string(enum tof type, Process *proc,
			  void* addr, size_t maxlen);
static int display_value(enum tof type, Process *proc,
			 long value, arg_type_info *info,
			 void *st, arg_type_info* st_info);
static int display_unknown(enum tof type, Process *proc, long value);
static int display_format(enum tof type, Process *proc, int arg_num);

static int string_maxlength = INT_MAX;
static int array_maxlength = INT_MAX;

static long
get_length(enum tof type, Process *proc, int len_spec,
		       void *st, arg_type_info* st_info) {
	long len;
	arg_type_info info;

	if (len_spec > 0)
		return len_spec;
	if (type == LT_TOF_STRUCT) {
		umovelong (proc, st + st_info->u.struct_info.offset[-len_spec-1],
			   &len, st_info->u.struct_info.fields[-len_spec-1]);
		return len;
	}

	info.type = ARGTYPE_INT;
	return gimme_arg(type, proc, -len_spec-1, &info);
}

static int
display_ptrto(enum tof type, Process *proc, long item,
			 arg_type_info * info,
			 void *st, arg_type_info* st_info) {
	arg_type_info temp;
	temp.type = ARGTYPE_POINTER;
	temp.u.ptr_info.info = info;
	return display_value(type, proc, item, &temp, st, st_info);
}

/*
 * addr - A pointer to the first element of the array
 *
 * The function name is used to indicate that we're not actually
 * looking at an 'array', which is a contiguous region of memory
 * containing a sequence of elements of some type; instead, we have a
 * pointer to that region of memory.
 */
static int
display_arrayptr(enum tof type, Process *proc,
			    void *addr, arg_type_info * info,
			    void *st, arg_type_info* st_info) {
	int len = 0;
	int i;
	int array_len;

	if (addr == NULL)
		return fprintf(options.output, "NULL");

	array_len = get_length(type, proc, info->u.array_info.len_spec,
			st, st_info);
	len += fprintf(options.output, "[ ");
	for (i = 0; i < options.arraylen && i < array_maxlength && i < array_len; i++) {
		arg_type_info *elt_type = info->u.array_info.elt_type;
		size_t elt_size = info->u.array_info.elt_size;
		if (i != 0)
			len += fprintf(options.output, ", ");
		if (options.debug)
			len += fprintf(options.output, "%p=", addr);
		len +=
			display_ptrto(type, proc, (long) addr, elt_type, st, st_info);
		addr += elt_size;
	}
	if (i < array_len)
		len += fprintf(options.output, "...");
	len += fprintf(options.output, " ]");
	return len;
}

/* addr - A pointer to the beginning of the memory region occupied by
 *        the struct (aka a pointer to the struct)
 */
static int
display_structptr(enum tof type, Process *proc,
			     void *addr, arg_type_info * info) {
	int i;
	arg_type_info *field;
	int len = 0;

	if (addr == NULL)
		return fprintf(options.output, "NULL");

	len += fprintf(options.output, "{ ");
	for (i = 0; (field = info->u.struct_info.fields[i]) != NULL; i++) {
		if (i != 0)
			len += fprintf(options.output, ", ");
		if (options.debug)
			len +=
				fprintf(options.output, "%p=",
						addr + info->u.struct_info.offset[i]);
		len +=
			display_ptrto(LT_TOF_STRUCT, proc,
					(long) addr + info->u.struct_info.offset[i],
					field, addr, info);
	}
	len += fprintf(options.output, " }");

	return len;
}

static int
display_pointer(enum tof type, Process *proc, long value,
			   arg_type_info * info,
			   void *st, arg_type_info* st_info) {
	long pointed_to;
	arg_type_info *inner = info->u.ptr_info.info;

	if (inner->type == ARGTYPE_ARRAY) {
		return display_arrayptr(type, proc, (void*) value, inner,
				st, st_info);
	} else if (inner->type == ARGTYPE_STRUCT) {
		return display_structptr(type, proc, (void *) value, inner);
	} else {
		if (value == 0)
			return fprintf(options.output, "NULL");
		else if (umovelong (proc, (void *) value, &pointed_to,
				    info->u.ptr_info.info) < 0)
			return fprintf(options.output, "?");
		else
			return display_value(type, proc, pointed_to, inner,
					st, st_info);
	}
}

static int
display_enum(enum tof type, Process *proc,
		arg_type_info* info, long value) {
	int ii;
	for (ii = 0; ii < info->u.enum_info.entries; ++ii) {
		if (info->u.enum_info.values[ii] == value)
			return fprintf(options.output, "%s", info->u.enum_info.keys[ii]);
	}

	return display_unknown(type, proc, value);
}

/* Args:
   type - syscall or shared library function or memory
   proc - information about the traced process
   value - the value to display
   info - the description of the type to display
   st - if the current value is a struct member, the address of the struct
   st_info - type of the above struct

   Those last two parameters are used for structs containing arrays or
   strings whose length is given by another structure element.
*/
int
display_value(enum tof type, Process *proc,
		long value, arg_type_info *info,
		void *st, arg_type_info* st_info) {
	int tmp;

	switch (info->type) {
	case ARGTYPE_VOID:
		return 0;
	case ARGTYPE_INT:
		return fprintf(options.output, "%d", (int) value);
	case ARGTYPE_UINT:
		return fprintf(options.output, "%u", (unsigned) value);
	case ARGTYPE_LONG:
		if (proc->mask_32bit)
			return fprintf(options.output, "%d", (int) value);
		else
			return fprintf(options.output, "%ld", value);
	case ARGTYPE_ULONG:
		if (proc->mask_32bit)
			return fprintf(options.output, "%u", (unsigned) value);
		else
			return fprintf(options.output, "%lu", (unsigned long) value);
	case ARGTYPE_OCTAL:
		return fprintf(options.output, "0%o", (unsigned) value);
	case ARGTYPE_CHAR:
		tmp = fprintf(options.output, "'");
		tmp += display_char(value == -1 ? value : (char) value);
		tmp += fprintf(options.output, "'");
		return tmp;
	case ARGTYPE_SHORT:
		return fprintf(options.output, "%hd", (short) value);
	case ARGTYPE_USHORT:
		return fprintf(options.output, "%hu", (unsigned short) value);
	case ARGTYPE_FLOAT: {
		union { long l; float f; double d; } cvt;
		cvt.l = value;
		return fprintf(options.output, "%f", cvt.f);
	}
	case ARGTYPE_DOUBLE: {
		union { long l; float f; double d; } cvt;
		cvt.l = value;
		return fprintf(options.output, "%lf", cvt.d);
	}
	case ARGTYPE_ADDR:
		if (!value)
			return fprintf(options.output, "NULL");
		else
			return fprintf(options.output, "0x%08lx", value);
	case ARGTYPE_FORMAT:
		fprintf(stderr, "Should never encounter a format anywhere but at the top level (for now?)\n");
		exit(1);
	case ARGTYPE_STRING:
		return display_string(type, proc, (void*) value,
				      string_maxlength);
	case ARGTYPE_STRING_N:
		return display_string(type, proc, (void*) value,
				      get_length(type, proc,
						 info->u.string_n_info.size_spec, st, st_info));
	case ARGTYPE_ARRAY:
		return fprintf(options.output, "<array without address>");
	case ARGTYPE_ENUM:
		return display_enum(type, proc, info, value);
	case ARGTYPE_STRUCT:
		return fprintf(options.output, "<struct without address>");
	case ARGTYPE_POINTER:
		return display_pointer(type, proc, value, info,
				       st, st_info);
	case ARGTYPE_UNKNOWN:
	default:
		return display_unknown(type, proc, value);
	}
}

int
display_arg(enum tof type, Process *proc, int arg_num, arg_type_info * info) {
	long arg;

	if (info->type == ARGTYPE_VOID) {
		return 0;
	} else if (info->type == ARGTYPE_FORMAT) {
		return display_format(type, proc, arg_num);
	} else {
		arg = gimme_arg(type, proc, arg_num, info);
		return display_value(type, proc, arg, info, NULL, NULL);
	}
}

static int
display_char(int what) {
	switch (what) {
	case -1:
		return fprintf(options.output, "EOF");
	case '\r':
		return fprintf(options.output, "\\r");
	case '\n':
		return fprintf(options.output, "\\n");
	case '\t':
		return fprintf(options.output, "\\t");
	case '\b':
		return fprintf(options.output, "\\b");
	case '\\':
		return fprintf(options.output, "\\\\");
	default:
		if (isprint(what)) {
			return fprintf(options.output, "%c", what);
		} else {
			return fprintf(options.output, "\\%03o", (unsigned char)what);
		}
	}
}

#define MIN(a,b) (((a)<(b)) ? (a) : (b))

static int
display_string(enum tof type, Process *proc, void *addr,
			  size_t maxlength) {
	unsigned char *str1;
	int i;
	int len = 0;

	if (!addr) {
		return fprintf(options.output, "NULL");
	}

	str1 = malloc(MIN(options.strlen, maxlength) + 3);
	if (!str1) {
		return fprintf(options.output, "???");
	}
	umovestr(proc, addr, MIN(options.strlen, maxlength) + 1, str1);
	len = fprintf(options.output, "\"");
	for (i = 0; i < MIN(options.strlen, maxlength); i++) {
		if (str1[i]) {
			len += display_char(str1[i]);
		} else {
			break;
		}
	}
	len += fprintf(options.output, "\"");
	if (str1[i] && (options.strlen <= maxlength)) {
		len += fprintf(options.output, "...");
	}
	free(str1);
	return len;
}

static int
display_unknown(enum tof type, Process *proc, long value) {
	if (proc->mask_32bit) {
		if ((int)value < 1000000 && (int)value > -1000000)
			return fprintf(options.output, "%d", (int)value);
		else
			return fprintf(options.output, "%p", (void *)value);
	} else if (value < 1000000 && value > -1000000) {
		return fprintf(options.output, "%ld", value);
	} else {
		return fprintf(options.output, "%p", (void *)value);
	}
}

static int
display_format(enum tof type, Process *proc, int arg_num) {
	void *addr;
	unsigned char *str1;
	int i;
	int len = 0;
	arg_type_info info;

	info.type = ARGTYPE_POINTER;
	addr = (void *)gimme_arg(type, proc, arg_num, &info);
	if (!addr) {
		return fprintf(options.output, "NULL");
	}

	str1 = malloc(MIN(options.strlen, string_maxlength) + 3);
	if (!str1) {
		return fprintf(options.output, "???");
	}
	umovestr(proc, addr, MIN(options.strlen, string_maxlength) + 1, str1);
	len = fprintf(options.output, "\"");
	for (i = 0; len < MIN(options.strlen, string_maxlength) + 1; i++) {
		if (str1[i]) {
			len += display_char(str1[i]);
		} else {
			break;
		}
	}
	len += fprintf(options.output, "\"");
	if (str1[i] && (options.strlen <= string_maxlength)) {
		len += fprintf(options.output, "...");
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
						len += fprintf(options.output, ", ...");
						str1[i + 1] = '\0';
						break;
					}
				} else if (c == 'd' || c == 'i') {
					info.type = ARGTYPE_LONG;
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(options.output, ", %d",
							    (int)gimme_arg(type, proc, ++arg_num, &info));
					else
						len +=
						    fprintf(options.output, ", %ld",
							    gimme_arg(type, proc, ++arg_num, &info));
					break;
				} else if (c == 'u') {
					info.type = ARGTYPE_LONG;
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(options.output, ", %u",
							    (int)gimme_arg(type, proc, ++arg_num, &info));
					else
						len +=
						    fprintf(options.output, ", %lu",
							    gimme_arg(type, proc, ++arg_num, &info));
					break;
				} else if (c == 'o') {
					info.type = ARGTYPE_LONG;
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(options.output, ", 0%o",
							    (int)gimme_arg(type, proc, ++arg_num, &info));
					else
						len +=
						    fprintf(options.output, ", 0%lo",
							    gimme_arg(type, proc, ++arg_num, &info));
					break;
				} else if (c == 'x' || c == 'X') {
					info.type = ARGTYPE_LONG;
					if (!is_long || proc->mask_32bit)
						len +=
						    fprintf(options.output, ", %#x",
							    (int)gimme_arg(type, proc, ++arg_num, &info));
					else
						len +=
						    fprintf(options.output, ", %#lx",
							    gimme_arg(type, proc, ++arg_num, &info));
					break;
				} else if (strchr("eEfFgGaACS", c)
					   || (is_long
					       && (c == 'c' || c == 's'))) {
					len += fprintf(options.output, ", ...");
					str1[i + 1] = '\0';
					break;
				} else if (c == 'c') {
					info.type = ARGTYPE_LONG;
					len += fprintf(options.output, ", '");
					len +=
					    display_char((int)
							 gimme_arg(type, proc, ++arg_num, &info));
					len += fprintf(options.output, "'");
					break;
				} else if (c == 's') {
					info.type = ARGTYPE_POINTER;
					len += fprintf(options.output, ", ");
					len +=
					    display_string(type, proc,
							   (void *)gimme_arg(type, proc, ++arg_num, &info),
							   string_maxlength);
					break;
				} else if (c == 'p' || c == 'n') {
					info.type = ARGTYPE_POINTER;
					len +=
					    fprintf(options.output, ", %p",
						    (void *)gimme_arg(type, proc, ++arg_num, &info));
					break;
				} else if (c == '*') {
					info.type = ARGTYPE_LONG;
					len +=
					    fprintf(options.output, ", %d",
						    (int)gimme_arg(type, proc, ++arg_num, &info));
				}
			}
		}
	}
	free(str1);
	return len;
}
