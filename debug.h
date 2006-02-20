#include <features.h>

void debug_(int level, const char *file, int line, const char *func, const char *fmt, ...);

int xwrite(const char *, size_t);
int xwrites(const char *);
int xwritehexi(int);
int xwritehexl(long);
int xwritec(char);
int xwritecr(void);
int xwritedump(void *, long, int);
int xinfdump(long, void *, int);


# define debug(level, expr...) debug_(level, __FILE__, __LINE__, DEBUG_FUNCTION, expr)

/* Version 2.4 and later of GCC define a magical variable `__PRETTY_FUNCTION__'
   which contains the name of the function currently being defined.
   This is broken in G++ before version 2.6.
   C9x has a similar variable called __func__, but prefer the GCC one since
   it demangles C++ function names.  */
# if __GNUC_PREREQ (2, 4)
#   define DEBUG_FUNCTION	__PRETTY_FUNCTION__
# else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#   define DEBUG_FUNCTION	__func__
#  else
#   define DEBUG_FUNCTION	((__const char *) 0)
#  endif
# endif
