Summary: Tracks runtime library calls for dynamically linked executables
Name: ltrace
%define version 0.3.19
Version: %{version}
Release: 1
Source: ftp://ftp.debian.org/debian/dists/unstable/main/source/utils/ltrace_%{version}.tar.gz
Copyright: GPL
Group: Development/Debuggers
BuildRoot: /tmp/ltrace-root

%description
ltrace is a debugging program which runs a specified command until it
exits.  While the command is executing, ltrace intercepts and records
the dynamic library calls which are called by
the executed process and the signals received by that process.
It can also intercept and print the system calls executed by the program.

The program to be traced need not be recompiled for this, so you can
use it on binaries for which you don't have the source handy.

You should install ltrace if you need a sysadmin tool for tracking the
execution of processes.

%prep 
%setup -q
./configure --prefix=/usr

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
rm -rf $RPM_BUILD_ROOT/usr/doc

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%config /etc/ltrace.conf
%doc COPYING README TODO BUGS ChangeLog
/usr/bin/ltrace
/usr/man/man1/ltrace.1
%changelog
