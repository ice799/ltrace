/* Ltrace Test : count-record.c.
   Objectives  : Verify that Ltrace can count all the system calls in
   execution and report a summary on exit.

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <errno.h>

void exit (int);

#define	BUF_SIZE	100

/*  Do as many operations as possible to record these calls.  */
int 
main ()
{
  FILE* fp;
  char s[]="system_calls";
  char buffer[BUF_SIZE];
  struct stat state;
	
  fp = fopen ("system_calls.tmp", "w");
  if (fp == NULL)
    {
      printf("Can not create system_calls.tmp\n");
      exit (0);
    }

  fwrite(s, sizeof(s), 1, fp);
  fseek (fp, 0, SEEK_CUR);
  fread(buffer, sizeof(s), 1, fp);
  fclose(fp);
  
  getcwd (buffer, BUF_SIZE);
  chdir (".");
  symlink ("system_calls.tmp", "system_calls.link");
  remove("system_calls.link");
  rename ("system_calls.tmp", "system_calls.tmp1");
  stat ("system_calls.tmp", &state);
  access ("system_calls.tmp", R_OK);
  remove("system_calls.tmp1");
  
  mkdir ("system_call_mkdir", 0777);
  rmdir ("system_call_mkdir");
  
  return 0;
}
