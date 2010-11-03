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

float func_float(float f1, float f2)
{
	printf("%f %f\n", f1, f2);
	return f1;
}

double func_double(double f1, double f2)
{
	printf("%f %f\n", f1, f2);
	return f2;
}

void func_typedef(int x)
{
	printf("typedef'd enum: %d\n", x);
}

void func_arrayi(int* a, int N)
{
    int i;
    printf("array[int]: ");
    for (i = 0; i < N; i++)
	printf("%d ", a[i]);
    printf("\n");
}

void func_arrayf(float* a, int N)
{
    int i;
    printf("array[float]: ");
    for (i = 0; i < N; i++)
	printf("%f ", a[i]);
    printf("\n");
}

struct test_struct {
    int simple;
    int alen;
    int slen;
    struct { int a; int b; }* array;
    struct { int a; int b; } seq[3];
    char* str;
    char* outer_str;
};

void func_struct(struct test_struct* x)
{
    char buf[100];
    int i;

    printf("struct: ");

    printf("%d, [", x->simple);
    for (i = 0; i < x->alen; i++) {
	printf("%d/%d", x->array[i].a, x->array[i].b);
	if (i < x->alen - 1)
	    printf(" ");
    }
    printf("] [");
    for (i = 0; i < 3; i++) {
	printf("%d/%d", x->seq[i].a, x->seq[i].b);
	if (i < 2)
	    printf(" ");
    }
    printf("] ");

    strncpy(buf, x->str, x->slen);
    buf[x->slen] = '\0';
    printf("%s\n", buf);
}
