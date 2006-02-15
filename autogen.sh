#!/bin/sh
#copied by ianw from ipbench (originally from oprofile 18-11-2003)

# run to generate needed files not in CVS

# NB: if you run this file with AUTOMAKE, AUTOCONF, etc. environment
# variables set, you *must* run "configure" with the same variables
# set. this is because "configure" will embed the values of these variables
# into the generated Makefiles, as @AUTOMAKE@, @AUTOCONF@ etc. and it will
# trigger regeneration of configuration state using those programs when any
# of Makefile.am etc. change.

run() {
	echo "Running $1 ..."
	$1
}

set -e

ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOMAKE=${AUTOMAKE:-automake}
AUTOCONF=${AUTOCONF:-autoconf}

if $AUTOMAKE --version | grep -q 1.4
	then
	echo ""
	echo "Automake 1.4 not supported. please set \$AUTOMAKE"
	echo "to point to a newer automake, or upgrade."
	echo ""
	exit 1
fi

if test -n "$1"; then
	echo "autogen.sh doesn't take any options" >&2
	exit 1
fi

run "$ACLOCAL"
run "$AUTOHEADER"
run "$AUTOCONF"
run "$AUTOMAKE --foreign --add-missing --copy"
