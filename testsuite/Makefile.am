# Copyright (C) 1992 - 2001 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 1, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

AUTOMAKE_OPTIONS = dejagnu

RUNTESTDEFAULTFLAGS = --srcdir $(srcdir)

SUBDIRS = ltrace.main ltrace.minor ltrace.torture

EXTRA_DIST = \
	README \
	config/unix.exp \
	lib/compiler.c \
	lib/compiler.cc \
	lib/ltrace.exp \
	run-my-tests.sh

CLEANFILES = *.o *.so *.log *.sum *.ltrace site.bak setval.tmp site.exp

MAINTAINERCLEANFILES = Makefile.in
