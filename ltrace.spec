Summary: Tracks runtime library calls from dynamically linked executables.
Name: ltrace
Version: 0.3.36
Release: 4.2
Source: ftp://ftp.debian.org/debian/pool/main/l/ltrace/ltrace_%{version}.orig.tar.gz
Patch1: ftp://ftp.debian.org/debian/pool/main/l/ltrace/ltrace_0.3.36-2.diff.gz
Patch2: ltrace-ppc64.patch
Patch3: ltrace-ppc64-2.patch
Patch4: ltrace-s390x.patch
Patch5: ltrace-syscallent-update.patch
Patch6: ltrace-fixes.patch
Patch7: ltrace-ia64.patch
License: GPL
Group: Development/Debuggers
ExclusiveArch: i386 x86_64 ia64 ppc ppc64 s390 s390x alpha sparc
Prefix: %{_prefix}
BuildRoot: /var/tmp/%{name}-root
BuildRequires: elfutils-libelf-devel

%description
Ltrace is a debugging program which runs a specified command until the
command exits.  While the command is executing, ltrace intercepts and
records both the dynamic library calls called by the executed process
and the signals received by the executed process.  Ltrace can also
intercept and print system calls executed by the process.

You should install ltrace if you need a sysadmin tool for tracking the
execution of processes.

%prep
%setup -q
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
sed -i -e 's/-o root -g root//' Makefile.in

%build
export CC="gcc`echo $RPM_OPT_FLAGS | sed -n 's/^.*\(-m[36][124]\).*$/ \1/p'`"
%configure CC="$CC"
make

%install
make DESTDIR=$RPM_BUILD_ROOT mandir=%{_mandir} install
rm -f ChangeLog; mv -f debian/changelog ChangeLog
rm -rf $RPM_BUILD_ROOT/%{_prefix}/doc

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING README TODO BUGS ChangeLog
%{_prefix}/bin/ltrace
%{_mandir}/man1/ltrace.1*
%config /etc/ltrace.conf

%changelog
* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 0.3.36-4.2
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 0.3.36-4.1
- rebuilt for new gcc4.1 snapshot and glibc changes

* Mon Jan  9 2006 Jakub Jelinek <jakub@redhat.com> 0.3.36-4
- added ppc64 and s390x support (IBM)
- added ia64 support (Ian Wienand)

* Sat Mar  5 2005 Jakub Jelinek <jakub@redhat.com> 0.3.36-3
- rebuilt with GCC 4

* Tue Dec 14 2004 Jakub Jelinek <jakub@redhat.com> 0.3.36-2
- make x86_64 ltrace trace both 32-bit and 64-bit binaries (#141955,
  IT#55600)
- fix tracing across execve
- fix printf-style format handling on 64-bit arches

* Thu Nov 18 2004 Jakub Jelinek <jakub@redhat.com> 0.3.36-1
- update to 0.3.36

* Mon Oct 11 2004 Jakub Jelinek <jakub@redhat.com> 0.3.35-1
- update to 0.3.35
- update syscall tables from latest kernel source

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Tue Jun  8 2004 Jakub Jelinek <jakub@redhat.com> 0.3.32-3
- buildreq elfutils-libelf-devel (#124921)

* Thu Apr 22 2004 Jakub Jelinek <jakub@redhat.com> 0.3.32-2
- fix demangling

* Thu Apr 22 2004 Jakub Jelinek <jakub@redhat.com> 0.3.32-1
- update to 0.3.32
  - fix dict.c assertion (#114359)
  - x86_64 support
- rewrite elf.[ch] using libelf
- don't rely on st_value of SHN_UNDEF symbols in binaries,
  instead walk .rel{,a}.plt and compute the addresses (#115299)
- fix x86-64 support
- some ltrace.conf additions
- some format string printing fixes

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Mon Feb  3 2003 Jakub Jelinek <jakub@redhat.com> 0.3.29-1
- update to 0.3.29

* Wed Jan 22 2003 Tim Powers <timp@redhat.com>
- rebuilt

* Sun Sep  1 2002 Jakub Jelinek <jakub@redhat.com> 0.3.10-12
- add a bunch of missing functions to ltrace.conf
  (like strlen, ugh)

* Fri Jun 21 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Tue May 28 2002 Phil Knirsch <pknirsch@redhat.com>
- Added the 'official' s390 patch.

* Thu May 23 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Wed Jan 09 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Fri Jul 20 2001 Jakub Jelinek <jakub@redhat.com>
- fix stale symlink in documentation directory (#47749)

* Sun Jun 24 2001 Elliot Lee <sopwith@redhat.com>
- Bump release + rebuild.

* Thu Aug  2 2000 Tim Waugh <twaugh@redhat.com>
- fix off-by-one problem in checking syscall number

* Wed Jul 12 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Mon Jun 19 2000 Matt Wilson <msw@redhat.com>
- rebuilt for next release
- patched Makefile.in to take a hint on mandir (patch2)
- use %%{_mandir} and %%makeinstall

* Wed Feb 02 2000 Cristian Gafton <gafton@redhat.com>
- fix description

* Fri Jan  7 2000 Jeff Johnson <jbj@redhat.com>
- update to 0.3.10.
- include (but don't apply) sparc patch from Jakub Jellinek.

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 2)

* Fri Mar 12 1999 Jeff Johnson <jbj@redhat.com>
- update to 0.3.6.

* Mon Sep 21 1998 Preston Brown <pbrown@redhat.com>
- upgraded to 0.3.4
