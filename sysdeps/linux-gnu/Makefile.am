DIST_SUBDIRS = \
	alpha \
	arm \
	i386 \
	ia64 \
	m68k \
	mipsel \
	ppc \
	s390 \
	sparc \
	x86_64

SUBDIRS = \
	$(HOST_CPU)

noinst_LTLIBRARIES = \
	../libos.la

___libos_la_SOURCES = \
	events.c \
	trace.c \
	proc.c \
	breakpoint.c

___libos_la_LIBADD = \
	libcpu.la

noinst_HEADERS = \
	arch_syscallent.h \
	signalent1.h \
	syscallent1.h

EXTRA_DIST = \
	arch_mksyscallent \
	mksignalent \
	mksyscallent \
	mksyscallent_s390

MAINTAINERCLEANFILES = \
	Makefile.in
