/* Ltrace Test : attach-process.c.
   Objectives  : Verify that ltrace can attach to a running process 
   by its PID.

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */

#include<unistd.h>
#include <sys/types.h>

int 
main ()
{
  sleep (5);
  sleep (1);
  return 0;
}
