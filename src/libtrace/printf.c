/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#include <stdarg.h>

static int printf(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[1024];

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);

	_sys_write(fd, buf, strlen(buf));

	return i;
}

