Summary: A library call tracer
Name: ltrace
%define version 0.3.7
Version: %{version}
Release: 1
Copyright: GPL
Group: Development/Debuggers
Source: ftp://ftp.debian.org/debian/dists/unstable/main/source/utils/ltrace_%{version}.tar.gz
BuildRoot: /tmp/ltrace

%description
ltrace is a library call tracer, i.e. a debugging tool which prints out
a trace of all the dynamic library calls made by another process/program.

It also displays system calls, as well as `strace', but strace still
does a better job displaying arguments to system calls.

The program to be traced need not be recompiled for this, so you can
use it on binaries for which you don't have the source handy.

This is still a work in progress, so some things may fail or don't work
as expected.

%changelog
%prep 
%setup

%build
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
gzip -9 $RPM_BUILD_ROOT/usr/doc/ltrace/ChangeLog

%clean
rm -rf $RPM_BUILD_ROOT

%files
%config /etc/ltrace.conf
/usr/bin/ltrace
/usr/doc/ltrace/ChangeLog.gz
/usr/doc/ltrace/BUGS
/usr/doc/ltrace/COPYING
/usr/doc/ltrace/README
/usr/doc/ltrace/TODO
/usr/man/man1/ltrace.1
