SUBDIRS = \
	sysdeps \
	. \
	testsuite

AM_CPPFLAGS = \
	$(libelf_CFLAGS) \
	-DSYSCONFDIR=\"$(sysconfdir)\"

noinst_LTLIBRARIES = \
	libltrace.la

libltrace_la_SOURCES = \
	breakpoints.c \
	debug.c \
	demangle.c \
	dict.c \
	display_args.c \
	ltrace-elf.c \
	execute_program.c \
	handle_event.c \
	libltrace.c \
	options.c \
	output.c \
	proc.c \
	read_config_file.c  \
	summary.c

libltrace_la_LIBADD = \
	$(libelf_LIBS) \
	$(liberty_LIBS) \
	$(libsupcxx_LIBS) \
	$(libunwind_LIBS) \
	$(libunwind_ptrace_LIBS) \
	$(libunwind_arch_LIBS) \
	sysdeps/libos.la


bin_PROGRAMS = \
	ltrace

ltrace_SOURCES = \
	main.c

ltrace_LDADD = \
	libltrace.la


noinst_HEADERS = \
	common.h \
	debug.h \
	defs.h \
	demangle.h \
	dict.h \
	ltrace-elf.h \
	ltrace.h \
	options.h \
	output.h \
	read_config_file.h

dist_man1_MANS = \
	ltrace.1

dist_doc_DATA = \
	COPYING \
	README \
	TODO \
	BUGS \
	ChangeLog

dist_sysconf_DATA = \
	etc/ltrace.conf

EXTRA_DIST = \
	ltrace.spec \
	debian/changelog \
	debian/compat \
	debian/control.in \
	debian/copyright \
	debian/rules

MAINTAINERCLEANFILES = \
	configure \
	Makefile.in \
	aclocal.m4 \
	config.h.in \
	config.h.in~ \
	config/autoconf/compile \
	config/autoconf/config.guess \
	config/autoconf/config.sub \
	config/autoconf/depcomp \
	config/autoconf/install-sh \
	config/autoconf/ltmain.sh \
	config/autoconf/mdate-sh \
	config/autoconf/missing \
	config/autoconf/texinfo.tex \
	libtool.m4 \
	ltoptions.m4 \
	ltsugar.m4 \
	ltversion.m4 \
	lt~obsolete.m4 \
	$(DIST_ARCHIVES)

maintainer-clean-local:
	-chmod -R a+rw $(distdir)
	-rm -fr $(distdir)
