/* Ltrace Test : trace-clone.c.
   Objectives  : Verify that ltrace can trace to child process after
   clone called.

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sched.h>

int child ()
{
  return 0;
}

typedef int (* myfunc)();

#define STACK_SIZE 1024

int main ()
{
  pid_t pid;
  static char stack[STACK_SIZE];
#ifdef __ia64__
  pid = __clone2((myfunc)&child, stack, STACK_SIZE, CLONE_FS, NULL);
#else
  pid = clone((myfunc)&child, stack,CLONE_FS, NULL );
#endif
  if (pid < 0)
    {
      perror("clone called failed");
      exit (1);
    }
  
  return 0;
}
