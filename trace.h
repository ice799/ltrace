#include "i386.h"

extern int pid;

int attach_process(const char * file, char * const argv[]);
void continue_process(int pid, int signal);
