# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)

AC_INIT([ltrace],[0.5.3],[ltrace-devel <ltrace-devel@lists.alioth.debian.org>])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR(libltrace.c)
#AC_CONFIG_MACRO_DIR([config/m4])
AC_CONFIG_AUX_DIR([config/autoconf])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

case "${host_os}" in
    linux-gnu*)	HOST_OS="linux-gnu" ;;
    *)		AC_MSG_ERROR([unkown host-os ${host_osx}]) ;;
esac
AC_SUBST(HOST_OS)

case "${host_cpu}" in
    arm*|sa110)		HOST_CPU="arm" ;;
    i?86)		HOST_CPU="i386" ;;
    powerpc|powerpc64)	HOST_CPU="ppc" ;;
    sun4u|sparc64)	HOST_CPU="sparc" ;;
    s390x)		HOST_CPU="s390" ;;
    *)			HOST_CPU="${host_cpu}" ;;
esac
AC_SUBST(HOST_CPU)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
# libtool-2:  LT_INIT()
AM_INIT_AUTOMAKE([foreign no-exeext dist-bzip2])
AM_MAINTAINER_MODE

# Checks for libraries.

# libelf
AC_CHECK_HEADERS([elf.h gelf.h],,
	[AC_MSG_ERROR([*** libelf.h or gelf.h not found on your system])]
)
AC_CHECK_LIB([elf], [elf_begin],,
	[AC_MSG_ERROR([*** libelf not found on your system])]
)


# HAVE_LIBIBERTY
AC_CHECK_LIB([iberty], [cplus_demangle], [
	AC_DEFINE([HAVE_LIBIBERTY], [1], [we have libiberty])
	liberty_LIBS="-liberty"], [
	liberty_LIBS=""])
AC_SUBST(liberty_LIBS)


# HAVE_LIBSUPC__
AC_CHECK_LIB([supc++], [__cxa_demangle], [
	AC_DEFINE([HAVE_LIBSUPC__], [1], [we have libsupc++])
	libsupcxx_LIBS="-lsupc++"], [
	libsupcxx_LIBS=""])
AC_SUBST(libsupcxx_LIBS)


# HAVE_LIBUNWIND
AC_ARG_WITH(libunwind,
  AS_HELP_STRING([--with-libunwind], [Use libunwind frame unwinding support]),
  [case "${withval}" in
  (yes|no) enable_libunwind=$withval;;
  (*) enable_libunwind=yes
    CPPFLAGS="${CPPFLAGS} -I${withval}/include"
    LDFLAGS="${LDFLAGS} -L${withval}/lib"
    ;;
esac],[enable_libunwind=maybe])

AC_CHECK_HEADERS([libunwind.h], [have_libunwind_h=yes])
AC_CHECK_HEADERS([libunwind-ptrace.h], [have_libunwind_ptrace_h=yes])

AC_MSG_CHECKING([whether to use libunwind support])
case "${enable_libunwind}" in
(yes|maybe)
  if test x$have_libunwind_h = xyes -o x$have_libunwind_ptrace_h = xyes; then
    enable_libunwind=yes
  elif test $enable_libunwind = maybe; then
    enable_libunwind=no
  else
    AC_MSG_RESULT([$enable_libunwind])
    AC_MSG_ERROR([libunwind.h or libunwind-ptrace.h cannot be found])	
  fi
  ;;
(*) ;;
esac
AC_MSG_RESULT([$enable_libunwind])

if test x"$enable_libunwind" = xyes; then
  AC_CHECK_LIB(unwind, backtrace, libunwind_LIBS=-lunwind, libunwind_LIBS=)
  AC_SUBST(libunwind_LIBS)
  AC_CHECK_LIB(unwind-ptrace, _UPT_create, libunwind_ptrace_LIBS=-lunwind-ptrace, libunwind_ptrace_LIBS=)
  AC_SUBST(libunwind_ptrace_LIBS)
  AC_CHECK_LIB(unwind-${HOST_CPU}, _U${HOST_CPU}_init_remote, libunwind_arch_LIBS=-lunwind-${HOST_CPU}, libunwind_arch_LIBS=)
  AC_SUBST(libunwind_arch_LIBS)
  AC_DEFINE([HAVE_LIBUNWIND], [1], [we have libunwind])
fi


# HAVE_ELF_C_READ_MMAP
AC_MSG_CHECKING([whether elf_begin accepts ELF_C_READ_MMAP])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gelf.h>]], [[
int main () {
	Elf *elf = elf_begin(4, ELF_C_READ_MMAP, 0);
	return 0;
}
	]])],[
	AC_DEFINE([HAVE_ELF_C_READ_MMAP], [1], [we have read mmap support])
	AC_MSG_RESULT([yes])],[
	AC_MSG_RESULT([no])])


CPPFLAGS=" \
	${CPPFLAGS} \
	-I\$(top_srcdir)/sysdeps/${HOST_OS}/${HOST_CPU} \
	-I\$(top_srcdir)/sysdeps/${HOST_OS} \
	-I\$(top_srcdir)/sysdeps \
	-I\$(top_srcdir) \
"

CFLAGS="${CFLAGS} -Wall"

# Checks for header files.
AC_CHECK_HEADERS([ \
	fcntl.h \
	limits.h \
	stddef.h \
	stdint.h \
	stdlib.h \
	string.h \
	sys/ioctl.h \
	sys/param.h \
	sys/time.h \
	unistd.h \
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF([long])


# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_CHECK_FUNCS([ \
	alarm \
	atexit \
	getcwd \
	gettimeofday \
	memset \
	mkdir \
	rmdir \
	strchr \
	strdup \
	strerror \
	strtol \
	strtoul \
])


#
# Debugging
#
AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug], [enable debugging @<:@default=no@:>@]),
	[case "$enableval" in
	y | yes) CONFIG_DEBUG=yes ;;
        *) CONFIG_DEBUG=no ;;
    esac],
    [CONFIG_DEBUG=no])
AC_MSG_RESULT([${CONFIG_DEBUG}])
if test "${CONFIG_DEBUG}" = "yes"; then
    CFLAGS="${CFLAGS} -Werror -Wsign-compare -Wfloat-equal -Wformat-security -g -O1"
    AC_DEFINE(DEBUG, 1, [debugging])
else
    CFLAGS="${CFLAGS} -O2"
fi

dnl 	testsuite/Makefile
dnl 	testsuite/ltrace.main/Makefile
dnl 	testsuite/ltrace.minor/Makefile
dnl 	testsuite/ltrace.torture/Makefile

AC_CONFIG_FILES([
	Makefile
	sysdeps/Makefile
	sysdeps/linux-gnu/Makefile
	sysdeps/linux-gnu/alpha/Makefile
	sysdeps/linux-gnu/arm/Makefile
	sysdeps/linux-gnu/i386/Makefile
	sysdeps/linux-gnu/ia64/Makefile
	sysdeps/linux-gnu/m68k/Makefile
	sysdeps/linux-gnu/mipsel/Makefile
	sysdeps/linux-gnu/ppc/Makefile
	sysdeps/linux-gnu/s390/Makefile
	sysdeps/linux-gnu/sparc/Makefile
	sysdeps/linux-gnu/x86_64/Makefile
	testsuite/Makefile
	testsuite/ltrace.main/Makefile
	testsuite/ltrace.minor/Makefile
	testsuite/ltrace.torture/Makefile
])
AC_OUTPUT
