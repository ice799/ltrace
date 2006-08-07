#include <string.h>
#include <stdio.h>

void func_ignore(int a, int b, int c)
{
	printf("%d\n", a + b + c);
}

void func_intptr(int *i)
{
	printf("%d\n", *i);
}

void func_intptr_ret(int *i)
{
	*i = 42;
}

int func_strlen(char* p)
{
	strcpy(p, "Hello world");
	return strlen(p);
}

void func_strfixed(char* p)
{
	strcpy(p, "Hello world");
}

void func_ppp(int*** ppp)
{
	printf("%d\n", ***ppp);
}

void func_stringp(char** sP)
{
	printf("%s\n", *sP);
}
