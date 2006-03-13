/* Ltrace Test : system_calls.c.
   Objectives  : Verify that Ltrace can trace all the system calls in
   execution.

   You can add new system calls in it and add its verification in 
   system_calls correspondingly.

   This file was written by Yao Qi <qiyao@cn.ibm.com>. */

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <errno.h>

void exit (int);

#define	BUF_SIZE	100

int 
main ()
{
  FILE* fp;
  char s[]="system_calls";
  char buffer[BUF_SIZE];
  struct stat state;
  
  /*  SYS_open.  */
  fp = fopen ("system_calls.tmp", "w");
  if (fp == NULL)
    {
      printf("Can not create system_calls.tmp\n");
      exit (0);
    }
  /*  SYS_write.  */
  fwrite(s, sizeof(s), 1, fp);
  /*  SYS_lseek.  */
  fseek (fp, 0, SEEK_CUR);
  /*  SYS_read.  */
  fread(buffer, sizeof(s), 1, fp);
  /*  SYS_close.  */
  fclose(fp);

  /*  SYS_getcwd.  */
  getcwd (buffer, BUF_SIZE);
  /*  SYS_chdir.  */
  chdir (".");
  /*  SYS_symlink.  */
  symlink ("system_calls.tmp", "system_calls.link");
  /*  SYS_unlink.  */
  remove("system_calls.link");
  /*  SYS_rename.  */
  rename ("system_calls.tmp", "system_calls.tmp1");
  /*  SYS_stat.  */
  stat ("system_calls.tmp", &state);
  /*  SYS_access.  */
  access ("system_calls.tmp", R_OK);
  remove("system_calls.tmp1");
  
  /*  SYS_mkdir.  */
  mkdir ("system_call_mkdir", 0777);
  /*  SYS_rmdir.  */
  rmdir ("system_call_mkdir");
  
  return 0;
}


