#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <ctype.h>

extern int attached_pid;
extern FILE * output;

#include "functions.h"

struct function * list_of_functions = NULL;

struct function functions_list[] = {
	{"atexit",      _T_INT,     1, {_T_ADDR}},
	{"close",       _T_INT,     1, {_T_INT}},
	{"exit",        _T_INT,     1, {_T_INT}},
	{"fclose",      _T_INT,     1, {_T_FILE}},
	{"fprintf",     _T_INT,     2, {_T_FILE, _T_FORMAT}},
	{"free",        _T_INT,     1, {_T_ADDR}},
	{"gethostname", _T_INT,     2, {_T_STRING, _T_INT}},
	{"getopt_long", _T_INT,     5, {_T_INT, _T_ADDR, _T_STRING, _T_ADDR, _T_ADDR}},
	{"malloc",      _T_ADDR,    1, {_T_UINT}},
	{"memset",      _T_ADDR,    3, {_T_ADDR, _T_CHAR, _T_UINT}},
	{"mkdir",       _T_INT,     2, {_T_STRING, _T_OCTAL}},
	{"open",        _T_INT,     3, {_T_STRING, _T_INT, _T_INT}},
	{"printf",      _T_INT,     1, {_T_FORMAT}},
	{"rindex",      _T_STRING,  2, {_T_STRING, _T_CHAR}},
	{"strcmp",      _T_INT,     2, {_T_STRING, _T_STRING}},
	{"strncmp",     _T_INT,     3, {_T_STRING, _T_STRING, _T_INT}},
	{"time",        _T_UINT,    1, {_T_ADDR}},
	{NULL,          _T_UNKNOWN, 5, {_T_UNKNOWN, _T_UNKNOWN, _T_UNKNOWN, _T_UNKNOWN, _T_UNKNOWN}},
};

static char * process_string(unsigned char * str)
{
	static char tmp[256];

	tmp[0] = '\0';
	while(*str) {
		switch(*str) {
			case '\r':	strcat(tmp,"\\r"); break;
			case '\n':	strcat(tmp,"\\n"); break;
			case '\t':	strcat(tmp,"\\t"); break;
			case '\\':	strcat(tmp,"\\"); break;
			default:
				if ((*str<32) || (*str>126)) {
					sprintf(tmp,"%s\\%03o", tmp, *str);
				} else {
					sprintf(tmp, "%s%c", tmp, *str);
				}
		}
		str++;
	}
	return tmp;
}

static char * print_string(int addr)
{
	static char tmp[256];
	int a;
	int i=0;

	tmp[0] = '\0';
	while(1) {
		a = ptrace(PTRACE_PEEKTEXT, attached_pid, addr+i, 0);
		*(int *)&tmp[i] = a;
		if (!tmp[i] || !tmp[i+1] || !tmp[i+2] || !tmp[i+3] || i>100) {
			break;
		}
		i += 4;
	}
	return process_string(tmp);
}

static char * print_param(int type, int esp)
{
	static char tmp[256];
	int a;

	a = ptrace(PTRACE_PEEKTEXT, attached_pid, esp, 0);

	switch(type) {
		case _T_STRING:
		case _T_FORMAT:
			sprintf(tmp,"\"%s\"",print_string(a));
			break;
		default:
			if (a<1000000 && a>-1000000) {
				sprintf(tmp, "%d", a);
			} else {
				sprintf(tmp, "0x%08x", a);
			}
	}
	return tmp;
}

void print_function(const char *name, int esp)
{
	struct function * tmp;
	char message[1024];
	int i;

	tmp = list_of_functions;
	while(tmp) {
		if (!strcmp(name, tmp->function_name)) {
			break;
		}
	}
	if (!tmp) {
		tmp = &functions_list[0];
		while(tmp->function_name) {
			if (!strcmp(name, tmp->function_name)) {
				break;
			}
			tmp++;
		}
	}
	sprintf(message, "%s(", name);
	if (tmp->num_params>0) {
		sprintf(message, "%s%s", message, print_param(tmp->params_type[0], esp+4));
	}
	for(i=1; i<tmp->num_params; i++) {
		sprintf(message, "%s,%s", message, print_param(tmp->params_type[i], esp+4*(i+1)));
	}
	fprintf(output, "%s) = ???\n", message);
	fflush(output);
}

static int func_type(char ** buf)
{
	int returned_value = 0;

	if (**buf == '+') {
		returned_value |= _T_OUTPUT;
		(*buf)++;
	}
	if (!strcmp(*buf, "int")) {
		returned_value |= _T_INT;
		*buf += 3;
	} else if (!strcmp(*buf, "addr")) {
		returned_value |= _T_ADDR;
		*buf += 4;
	} else {
		return -1;
	}
	return returned_value;
}

static int fill_fields(struct function * fun, char * buf)
{
	char * tmp = buf;

	for(; (*tmp==' '); tmp++);
	if ((fun->return_type = func_type(&buf)) == -1) {
		return 0;
	}
	for(; (*tmp==' '); tmp++);

	return 0;
}

void read_config_file(const char * filename)
{
	char buf[1024];
	FILE * stream;
	struct function tmp;

	stream = fopen(filename, "r");
	if (!stream) {
		return;
	}
	while (fgets(buf, 1024, stream)) {
		if (fill_fields(&tmp, buf)) {
			tmp.next = list_of_functions;
			list_of_functions = malloc(sizeof(struct function));
			bcopy(&tmp, list_of_functions, sizeof(struct function));
		}
	}
}
