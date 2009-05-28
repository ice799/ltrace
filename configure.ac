dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)
AC_INIT(ltrace,0.5.2,[ltrace-devel <ltrace-devel@lists.alioth.debian.org>])
AC_CONFIG_SRCDIR(ltrace.c)
AC_CONFIG_HEADER(config.h)

dnl Check host system type
AC_CANONICAL_HOST
case "$host_os" in
  linux-gnu*) host_os=linux-gnu
esac
HOST_OS="$host_os"
AC_SUBST(HOST_OS)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for libraries.
AC_CHECK_LIB(iberty, cplus_demangle,,,)
AC_CHECK_LIB(supc++, __cxa_demangle,,,)

dnl
dnl The following stuff may be useful, but I don't use it now.
dnl

dnl   dnl Checks for header files.
AC_HEADER_STDC
dnl   AC_HEADER_SYS_WAIT
dnl   AC_CHECK_HEADERS(fcntl.h limits.h sys/time.h unistd.h)
AC_CHECK_HEADERS(getopt.h)
dnl   
dnl   dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_PID_T
dnl   AC_HEADER_TIME
dnl   AC_STRUCT_TM
dnl   
dnl   dnl Checks for library functions.
dnl   AC_FUNC_MMAP
dnl   AC_TYPE_SIGNAL
dnl   AC_FUNC_VPRINTF
dnl   AC_CHECK_FUNCS(gettimeofday strdup strerror strspn)
AC_CHECK_FUNCS(getopt getopt_long)

for path in /usr/include/elfutils /usr/local/include/elfutils \
	    /usr/include/libelf /usr/local/include/libelf; do
	if test -f ${path}/gelf.h; then
		CPPFLAGS="$CPPFLAGS -I ${path}"
	fi
done

AC_CHECK_HEADER(gelf.h,,AC_MSG_ERROR([***** gelf.h not found *****]))

AC_CHECK_LIB(elf,gelf_getdyn)

ac_cv_have_iquote=no
AC_MSG_CHECKING(compiler accepts -iquote dir)
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -iquote ."
AC_TRY_COMPILE(,,[ac_cv_have_iquote=yes])
CPPFLAGS="$save_CPPFLAGS"
AC_MSG_RESULT($ac_cv_have_iquote)
if test $ac_cv_have_iquote = yes; then
  iquote="-iquote "
  iquoteend=""
else
  iquote="-I "
  iquoteend="-I-"
fi
AC_SUBST(iquote)
AC_SUBST(iquoteend)

ac_cv_have_elf_c_read_mmap=no
AC_MSG_CHECKING(elf_begin accepts ELF_C_READ_MMAP)
AC_TRY_COMPILE([#include <gelf.h>],
	       [Elf *elf = elf_begin (4, ELF_C_READ_MMAP, 0);],
	       [ac_cv_have_elf_c_read_mmap=yes])
AC_MSG_RESULT($ac_cv_have_elf_c_read_mmap)
if test $ac_cv_have_elf_c_read_mmap = yes; then
AC_DEFINE(HAVE_ELF_C_READ_MMAP, 1,
	  [Defined if ELF_C_READ_MMAP is valid argument for elf_begin])
dnl This is not 100% correct, but all elfutils installations
dnl are built with LFS, while AFAIK all Michael Riepe's libelfs
dnl are built without LFS.  It is important that LFS setting
dnl of libelf and ltrace matches.
AC_SYS_LARGEFILE
fi
AC_OUTPUT(Makefile testsuite/Makefile \
	  testsuite/ltrace.main/Makefile testsuite/ltrace.minor/Makefile \
	  testsuite/ltrace.torture/Makefile )

