#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "ltrace.h"
#include "options.h"

static int display_char(int what);
static int display_string(enum tof type, struct process * proc, int arg_num);
static int display_stringN(int arg2, enum tof type, struct process * proc, int arg_num);
static int display_unknown(enum tof type, struct process * proc, int arg_num);
static int display_format(enum tof type, struct process * proc, int arg_num);

int display_arg(enum tof type, struct process * proc, int arg_num, enum param_type rt)
{
	int tmp;
	long arg;

	switch(rt) {
		case LT_PT_VOID:
			return 0;
		case LT_PT_INT:
			return fprintf(output, "%d", (int)gimme_arg(type, proc, arg_num));
		case LT_PT_UINT:
			return fprintf(output, "%u", (unsigned)gimme_arg(type, proc, arg_num));
		case LT_PT_OCTAL:
			return fprintf(output, "0%o", (unsigned)gimme_arg(type, proc, arg_num));
		case LT_PT_CHAR:
			tmp = fprintf(output, "'");
			tmp += display_char((int)gimme_arg(type, proc, arg_num));
			tmp += fprintf(output, "'");
			return tmp;
		case LT_PT_ADDR:
			arg = gimme_arg(type, proc, arg_num);
			if (!arg) {
				return fprintf(output, "NULL");
			} else {
				return fprintf(output, "0x%08x", (unsigned)arg);
			}
		case LT_PT_FORMAT:
			return display_format(type, proc, arg_num);
		case LT_PT_STRING:
			return display_string(type, proc, arg_num);
		case LT_PT_STRING0:
			return display_stringN(0, type, proc, arg_num);
		case LT_PT_STRING1:
			return display_stringN(1, type, proc, arg_num);
		case LT_PT_STRING2:
			return display_stringN(2, type, proc, arg_num);
		case LT_PT_STRING3:
			return display_stringN(3, type, proc, arg_num);
		case LT_PT_UNKNOWN:
		default:
			return display_unknown(type, proc, arg_num);
	}
	return fprintf(output, "?");
}

static int display_char(int what)
{
	switch(what) {
		case -1:	return fprintf(output, "EOF");
		case '\r':	return fprintf(output, "\\r");
		case '\n':	return fprintf(output, "\\n");
		case '\t':	return fprintf(output, "\\t");
		case '\b':	return fprintf(output, "\\b");
		case '\\':	return fprintf(output, "\\");
		default:
			if ((what<32) || (what>126)) {
				return fprintf(output, "\\%03o", (unsigned char)what);
			} else {
				return fprintf(output, "%c", what);
			}
	}
}

static int string_maxlength=INT_MAX;

#define MIN(a,b) (((a)<(b)) ? (a) : (b))

static int display_string(enum tof type, struct process * proc, int arg_num)
{
	void * addr;
	char * str1;
	int i;
	int len=0;

	addr = (void *)gimme_arg(type, proc, arg_num);
	if (!addr) {
		return fprintf(output, "NULL");
	}

	str1 = malloc(MIN(opt_s,string_maxlength)+3);
	if (!str1) {
		return fprintf(output, "???");
	}
	umovestr(proc, addr, MIN(opt_s,string_maxlength)+1, str1);
	len = fprintf(output, "\"");
	for(i=0; i<MIN(opt_s,string_maxlength); i++) {
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
	free(str1);
	return len;
}

static int display_stringN(int arg2, enum tof type, struct process * proc, int arg_num)
{
	int a;

	string_maxlength=gimme_arg(type, proc, arg2-1);
	a = display_string(type, proc, arg_num);
	string_maxlength=INT_MAX;
	return a;
}

static int display_unknown(enum tof type, struct process * proc, int arg_num)
{
	long tmp;

	tmp = gimme_arg(type, proc, arg_num);

	if (tmp<1000000 && tmp>-1000000) {
		return fprintf(output, "%ld", tmp);
	} else {
		return fprintf(output, "0x%08lx", tmp);
	}
}

static int display_format(enum tof type, struct process * proc, int arg_num)
{
	void * addr;
	char * str1;
	int i;
	int len=0;

	addr = (void *)gimme_arg(type, proc, arg_num);
	if (!addr) {
		return fprintf(output, "NULL");
	}

	str1 = malloc(MIN(opt_s,string_maxlength)+3);
	if (!str1) {
		return fprintf(output, "???");
	}
	umovestr(proc, addr, MIN(opt_s,string_maxlength)+1, str1);
	len = fprintf(output, "\"");
	for(i=0; len<MIN(opt_s,string_maxlength)+1; i++) {
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
	for(i=0; str1[i]; i++) {
		if (str1[i]=='%') {
			while(1) {
				char c = str1[++i];
				if (c == '%') {
					break;
				} else if (!c) {
					break;
				} else if ((c=='d') || (c=='i')) {
					len += fprintf(output, ", %d", (int)gimme_arg(type, proc, ++arg_num));
					break;
				} else if (c=='u') {
					len += fprintf(output, ", %u", (int)gimme_arg(type, proc, ++arg_num));
					break;
				} else if (c=='o') {
					len += fprintf(output, ", 0%o", (int)gimme_arg(type, proc, ++arg_num));
					break;
				} else if ((c=='x') || (c=='X')) {
					len += fprintf(output, ", 0x%x", (int)gimme_arg(type, proc, ++arg_num));
					break;
				} else if (c=='c') {
					len += fprintf(output, ", '");
					len += display_char((int)gimme_arg(type, proc, ++arg_num));
					len += fprintf(output, "'");
					break;
				} else if (c=='s') {
					len += fprintf(output, ", ");
					len += display_string(type, proc, ++arg_num);
					break;
				} else if ((c=='e') || (c=='E') || (c=='f') || (c=='g')) {
					len += fprintf(output, ", ...");
					str1[i+1]='\0';
					break;
				} else if (c=='*') {
					len += fprintf(output, ", %d", (int)gimme_arg(type, proc, ++arg_num));
				}
			}
		}
	}
	free(str1);
	return len;
}
