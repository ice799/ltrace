/* Copyright (C) 1993  Olaf Flebbe
This file is part of the Linux C Library.
*/

#include <fpu_control.h>

void
__setfpucw(unsigned short fpu_control)
{
  volatile unsigned short cw;

	init_libtrace();

  /* If user supplied _fpu_control, use it ! */
  if (!fpu_control)
  { 
    /* use linux defaults */
    fpu_control = _FPU_DEFAULT;
  }
  /* Get Control Word */
  __asm__ volatile ("fnstcw %0" : "=m" (cw) : );
  
  /* mask in */
  cw &= _FPU_RESERVED;
  cw = cw | (fpu_control & ~_FPU_RESERVED);

  /* set cw */
  __asm__ volatile ("fldcw %0" :: "m" (cw));
}
