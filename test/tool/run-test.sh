#!/bin/bash

if gmake --version > /dev/null 2>&1; then
    MAKE=${MAKE:-"gmake"}
else
    MAKE=${MAKE:-"make"}
fi

if test -z "$abs_top_builddir"; then
    abs_top_builddir="$(${MAKE} -s echo-abs-top-builddir)"
fi

BASE_DIR="$abs_top_builddir/test/tool"

if test -z "$RUBY"; then
    RUBY="$(${MAKE} -s -C "$abs_top_builddir" echo-ruby)"
fi

${RUBY} ${BASE_DIR}/run-test.rb
