/* Ltrace Test : time-record.c.
   Objectives  : Verify that Ltrace can record timestamp and spent
   time inside each call.

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */
#include <stdio.h>
#include <time.h>

#define SLEEP_COUNT 2
#define NANOSLEEP_COUNT 50

int 
main ()
{
  struct timespec request, remain;
  request.tv_sec = 0;
  request.tv_nsec = NANOSLEEP_COUNT * 1000000;
  
  sleep (SLEEP_COUNT);
  nanosleep (&request, NULL);
  
  return 0;
}
