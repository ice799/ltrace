#include<stddef.h>
#include<iostream>
#include<cstdlib>
#include"demangle.h"

/* Number of arguments */
int Fi_i(int bar) 		{ return 0; }
int Fi_s(short bar) 		{return 0; }
int Fii_i(int bar, int goo) 	{ return 0; }
int Fiii_i(int bar, int goo, int hoo) { return 0; }
int Fie_i(int bar, ...) 	{ return 0; }

/* Return types */
void Fv_v(void) 		{ ; }
char Fv_c(void) 		{ return 0; }
signed char Fv_Sc(void) 	{ return 0; }
unsigned char Fv_Uc(void) 	{ return 0; }
short Fv_s(void) 		{ return 0; }
unsigned short Fv_Us(void) 	{ return 0; }
int Fv_i(void) 			{ return 0; }
const int Fv_Ci(void) 		{ return 0; }
unsigned int Fv_Ui(void) 	{ return 0; }
volatile int Fv_Vi(void) 	{ return 0; }
long Fv_l(void) 		{ return 0; }
unsigned long Fv_Ul(void) 	{ return 0; }
float Fv_f(void) 		{ return 0; }
double Fv_g(void) 		{ return 0; }
long double Fv_Lg(void) 	{ return 0; }

/* Pointers */
void *Fv_Pv(void) 		{ return 0; }
void **Fv_PPv(void) 		{ return 0; }

/* References */
int& Fv_Ri(void) 		{ static int x; return x; }

/* Argument types */
int FPi_i(int *a) 		{ return 0; }
int FA10_i_i(int a[10]) 	{ return 0; }
int Fc_i(char bar) 		{ return 0; }
int Ff_i(float bar) 		{ return 0; }
int Fg_i(double bar) 		{ return 0; }

/* Function pointers */
typedef int (*x)(int);
typedef int (*y)(short);

int Fx_i(x fnptr) 		{ return 0; }
int Fxx_i(x fnptr, x fnptr2) 	{ return 0; }
int Fxxx_i(x fnptr, x fnptr2, 
	x fnptr3) 		{ return 0; }
int Fxxi_i(x fnptr, x fnptr2, 
	x fnptr3, int i) 	{ return 0; }
int Fxix_i(x fnptr, int i, 
	x fnptr3) 		{ return 0; }
int Fxyxy_i(x fnptr, y fnptr2, 
	x fnptr3, y fnptr4) 	{ return 0; }

/* Class methods */
class myclass;
myclass::myclass(void) 		{ myint = 0; }
myclass::myclass(int x) 	{ myint = x; }
myclass::~myclass() 		{ ; }

int myclass::Fi_i(int bar) 	{ return myint; }
int myclass::Fis_i(int bar) 	{ return bar; }

void* myclass::operator new(size_t size)
{
  void* p = malloc(size);return p;
}
void myclass::operator delete(void *p) {free(p);}

myclass myclass::operator++() 	{ return myclass(++myint); }
myclass myclass::operator++(int) { return myclass(myint++); }

/* Binary */
myclass myclass::operator+(int x) { return myclass(myint + x); }

/* Assignment */
myclass& myclass::operator=(const myclass& from) 
{ 
	myint = from.myint; 
	return *this; 
}

/* test clashes */
class nested;

nested::nested(void) 		{ ; }
nested::~nested() 		{ ; }
int nested::Fi_i(int bar) 	{ return bar; }

void Fmyclass_v(myclass m) 	{ ; }
void Fmxmx_v(myclass arg1, x arg2, 
	myclass arg3, x arg4) 	{ ; }

