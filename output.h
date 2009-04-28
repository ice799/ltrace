#include <sys/types.h>

#include "ltrace.h"

void output_line(Process *proc, char *fmt, ...);

void output_left(enum tof type, Process *proc, char *function_name);
void output_right(enum tof type, Process *proc, char *function_name);
