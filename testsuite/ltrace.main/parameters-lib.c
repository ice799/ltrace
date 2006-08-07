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

void func_enum(int x)
{
	printf("enum: %d\n", x);
}

void func_short(short x1, short x2)
{
	printf("short: %hd %hd\n", x1, x2);
}

void func_ushort(unsigned short x1, unsigned short x2)
{
	printf("ushort: %hu %hu\n", x1, x2);
}

void func_float(float f1, float f2)
{
	printf("%f %f\n", f1, f2);
}

void func_typedef(int x)
{
	printf("typedef'd enum: %d\n", x);
}
