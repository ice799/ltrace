#! /bin/sh
bitmode=""

# This shell script is used to run the ltrace test suite.  It is possible to
# run it via 'make check' using RUNTESTFLAGS.  This script just makes it easy.

function usage
{
  echo usage: `basename $0` '-m32|-m64 [<tool> | ""] [<test.exp>]'
}

# The first argument is not optional: it must either be -m32 or -m64.  If the
# second argument is used, it specifies the file name of the ltrace to be
# tested.  The third argument specifies a particular test case to run.  If
# the third argument is omitted, then all test cases are run.  If you wish to
# use the third argument, but not the second, specify the second as "".

# there is a secret argument: if the name of this script is 'test', then
# the --verbose argument is added to RUNTESTFLAGS.

if [ x"$1" == x -o x"$1" != x-m32 -a x"$1" != x-m64 ]; then
  usage
  exit 1
fi

flags=''

if [ `basename $0` == test ]; then
  flags="--verbose "
fi

if [ x"$2" != x ]; then
  flags="${flags}--tool_exec=$2 "
fi

flags="${flags}CFLAGS_FOR_TARGET=$1"

if [ x"$3" != x ]; then
  flags="$flags $3"
fi

set -o xtrace
make check RUNTESTFLAGS="$flags"
