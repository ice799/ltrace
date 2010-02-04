#!/bin/bash

set -e

#
# usage:
#
# banner <target name>
#
banner() {
	echo
	TG=`echo $1 | sed -e "s,/.*/,,g"`
	LINE=`echo $TG |sed -e "s/./-/g"`
	echo $LINE
	echo $TG
	echo $LINE
	echo
}

banner "autoreconf"

mkdir -p config/autoconf config/m4
autoreconf --force --install --symlink -Wall || exit $?

banner "Finished"
