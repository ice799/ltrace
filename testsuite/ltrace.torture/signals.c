/* Ltrace Test : signals.c.
   Objectives  : Verify that ltrace can trace user defined signal.
   This file was written by Yao Qi <qiyao@cn.ibm.com>. */

#include<stdio.h>
#include<signal.h>
#include <sys/types.h>

#define LOOP	20

void 
handler(int signum,siginfo_t *info,void *act)
{
}

int 
main ()
{
  struct sigaction act;	
  union sigval mysigval;
  int i;
  int sig;
  pid_t pid;
  
  mysigval.sival_int=0;
  sig = SIGUSR1;
  pid=getpid();
  
  sigemptyset(&act.sa_mask);
  act.sa_sigaction=handler;
  act.sa_flags=SA_SIGINFO;
  
  if(sigaction(sig,&act,NULL) < 0)
    {
      printf("install sigal error\n");
    }
  
  for(i=0; i< LOOP; i++)
    {
      usleep(100);
      sigqueue(pid,sig,mysigval);
    }
  return 0;
}
