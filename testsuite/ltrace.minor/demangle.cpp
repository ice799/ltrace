/* Ltrace Test : demangle.cpp
   Objectives  : Verify that ltrace can demangle C++ symbols.

   This file was written by Yao Qi <qiyao@cn.ibm.com>.  */

#include<stddef.h>
#include"demangle.h"

/* Number of arguments */
extern int Fi_i(int);
extern int Fi_s (short);
extern int Fii_i(int , int);
extern int Fiii_i(int, int, int);
extern int Fie_i(int bar, ...);

/* Return types */
extern void Fv_v(void);
extern char Fv_c(void);
extern signed char Fv_Sc(void);
extern unsigned char Fv_Uc(void);
extern short Fv_s(void);
extern unsigned short Fv_Us(void);
extern int Fv_i(void);
extern const int Fv_Ci(void);
extern unsigned int Fv_Ui(void);
extern volatile int Fv_Vi(void);
extern long Fv_l(void);
extern unsigned long Fv_Ul(void);
extern float Fv_f(void) ;
extern double Fv_g(void);
extern long double Fv_Lg(void);


/* Pointers */
extern void * Fv_Pv(void);
extern void ** Fv_PPv(void);

/* References */
extern int& Fv_Ri(void);

/* Argument types */
extern int FPi_i(int *a);
extern int FA10_i_i(int a[10]) ;
extern int Fc_i(char bar);
extern int Ff_i(float bar);
extern int Fg_i(double bar);

/* Function pointers */
typedef int (*x)(int);
typedef int (*y)(short);

extern int Fx_i(x);
extern int Fxx_i(x fnptr, x fnptr2);
extern int Fxxx_i(x fnptr, x fnptr2, x fnptr3);
extern int Fxxi_i(x fnptr, x fnptr2, x fnptr3, int i);
extern int Fxix_i(x fnptr, int i, x fnptr3);
extern int Fxyxy_i(x fnptr, y fnptr2, x fnptr3, y fnptr4);


extern void Fmyclass_v(myclass m);
extern void Fmxmx_v(myclass arg1, x arg2, myclass arg3, x arg4);

int main ()
{
  int i;

  i = Fi_i (0);
  i = Fii_i (0,0);
  i = Fiii_i (0,0,0);
  i = Fie_i (0);

  Fv_v ();
  Fv_c ();
  Fv_Sc ();
  Fv_Uc ();
  Fv_s ();
  Fv_Us ();
  Fv_i ();
  Fv_Ci ();
  Fv_Ui ();
  Fv_Vi ();
  Fv_l ();
  Fv_Ul ();
  Fv_f ();
  Fv_g ();
  Fv_Lg ();

  Fv_Pv ();
  Fv_PPv ();
	
  Fv_Ri ();

  FPi_i (&i);
  FA10_i_i (&i);
  Fc_i ('a');
  Ff_i (1.1f);
  Fg_i (1.1);

  Fx_i (Fi_i);
  Fxx_i (Fi_i, Fi_i);
  Fxxx_i (Fi_i, Fi_i, Fi_i);
  Fxxi_i (Fi_i, Fi_i, Fi_i, 0);
  Fxyxy_i (Fi_i, Fi_s, Fi_i, Fi_s);

  myclass a,c;
  myclass* b;
  
  a.Fi_i (0);
  a.Fis_i (0);
  a++;
  c = a + 2;
  
  nested n;  
  n.Fi_i (0);

  b = (myclass*) new myclass(0);
  delete (b);
  Fmyclass_v (a);

  return 0;
}
