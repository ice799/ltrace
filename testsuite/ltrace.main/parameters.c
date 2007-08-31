/* Ltrace Test : parameters.c.
   Objectives  : Verify that Ltrace can handle all the different
   parameter types

   This file was written by Steve Fink <sphink@gmail.com>. */

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void func_intptr(int *i);
void func_intptr_ret(int *i);
int func_strlen(char*);
void func_strfixed(char*);
void func_ppp(int***);
void func_stringp(char**);
void func_short(short, short);
void func_ushort(unsigned short, unsigned short);
void func_float(float, float);
void func_arrayi(int*, int);
void func_arrayf(float*, int);
void func_struct(void*);

typedef enum {
  RED,
  GREEN,
  BLUE,
  CHARTREUSE,
  PETUNIA
} color_t;
void func_enum(color_t);
void func_typedef(color_t);

int 
main ()
{
  int x = 17;
  int *xP, **xPP;
  char buf[200];
  char *s;
  int *ai;
  float *af;

  func_intptr(&x);

  func_intptr_ret(&x);

  func_strlen(buf);
  printf("%s\n", buf);

  func_strfixed(buf);
  printf("%s\n", buf);

  x = 80;
  xP = &x;
  xPP = &xP;
  func_ppp(&xPP);

  s = (char*) malloc(100);
  strcpy(s, "Dude");
  func_stringp(&s);

  func_enum(BLUE);

  func_short(-8, -9);
  func_ushort(33, 34);
  func_float(3.4, -3.4);

  func_typedef(BLUE);

  ai = (int*) calloc(sizeof(int), 8);
  for (x = 0; x < 8; x++)
    ai[x] = 10 + x;
  func_arrayi(ai, 8);
  func_arrayi(ai, 2);

  af = (float*) calloc(sizeof(float), 8);
  for (x = 0; x < 8; x++)
    af[x] = 10.1 + x;
  func_arrayf(af, 8);
  func_arrayf(af, 2);

  {
    struct {
      int simple;
      int alen;
      int slen;
      struct { int a; int b; }* array;
      struct { int a; int b; } seq[3];
      char* str;
    } x;

    x.simple = 89;

    x.alen = 2;
    x.array = malloc(800);
    x.array[0].a = 1;
    x.array[0].b = 10;
    x.array[1].a = 3;
    x.array[1].b = 30;

    x.seq[0].a = 4;
    x.seq[0].b = 40;
    x.seq[1].a = 5;
    x.seq[1].b = 50;
    x.seq[2].a = 6;
    x.seq[2].b = 60;

    x.slen = 3;
    x.str = "123junk";

    func_struct(&x);
  }

  return 0;
}
