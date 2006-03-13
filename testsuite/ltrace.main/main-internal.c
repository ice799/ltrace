 /* Ltrace Test : main-internal.c.
    Objectives  : Verify that ltrace can trace call from main 
    executable within it.
    This file was written by Yao Qi <qiyao@cn.ibm.com>. */
#include<stdio.h>

extern void display ( char* );

#define DISPLAY_LOOP	12

int 
main ()
{
	int i;
	printf ("should not show up if '-X display' is used.\n");
	for (i=0; i< DISPLAY_LOOP; i++)
		display ("Function call within executable.");
}

