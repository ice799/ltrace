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

int main ()
{
  pid_t pid;
  static char stack[1024];
  
  if ((pid = clone((myfunc)&child, stack,CLONE_FS, NULL )) < 0)
    {
      perror("clone called failed");
      exit (1);
    }
  
  return 0;
}
