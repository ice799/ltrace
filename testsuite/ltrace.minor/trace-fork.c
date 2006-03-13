/* Ltrace Test : trace-fork.c
   Objectives  : Verify that ltrace can trace to child process after
   fork called.

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */

#include <stdio.h>
#include <sys/types.h>

void 
child ()
{
  printf("Fork Child\n");
  sleep(1);
}

int 
main ()
{
  pid_t pid;
  pid = fork ();
  
  if (pid == -1)
    printf("fork failed!\n");
  else if (pid == 0)
    child();
  else
    {
      printf("My child pid is %d\n",pid);
      wait(); 
    }
  return 0;
}
