#!/bin/bash

unset MAKELEVEL
unset MAKEFLAGS

if gmake --version > /dev/null 2>&1; then
    MAKE=${MAKE:-"gmake"}
else
    MAKE=${MAKE:-"make"}
fi

if test -z "$abs_top_srcdir"; then
    abs_top_srcdir="$(${MAKE} -s echo-abs-top-srcdir)"
fi

BASE_DIR="$abs_top_srcdir/test/tool"

if test -z "$RUBY"; then
    RUBY="$(${MAKE} -s echo-ruby)"
fi

${RUBY} ${BASE_DIR}/run-test.rb "$@"
