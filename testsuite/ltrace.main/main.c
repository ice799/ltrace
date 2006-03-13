/* Ltrace Test : main.c.
   Objectives  : Verify that ltrace can trace call a library function 
   from main executable.  

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */

extern void print ( char* );

#define	PRINT_LOOP	10

int
main ()
{
  int i;

  for (i=0; i<PRINT_LOOP; i++)
    print ("Library function call!");
  
  return 0;
}

