#!/bin/bash
#
# Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

unset MAKELEVEL
unset MAKEFLAGS

if gmake --version > /dev/null 2>&1; then
    MAKE=${MAKE:-"gmake"}
else
    MAKE=${MAKE:-"make"}
fi

if test -z "$abs_top_builddir"; then
    abs_top_builddir="$(${MAKE} -s echo-abs-top-builddir)"
fi

if test -z "$abs_top_srcdir"; then
    abs_top_srcdir="$(${MAKE} -s -C "$abs_top_builddir" echo-abs-top-srcdir)"
fi

export BASE_DIR="$abs_top_builddir/test"
export SOURCE_DIR="$abs_top_srcdir"
export BUILD_DIR="$abs_top_builddir"

if test x"$NO_MAKE" != x"yes"; then
    $MAKE -C "$abs_top_builddir" > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="$(${MAKE} -s -C "$abs_top_builddir" echo-cutter)"
fi

CUTTER_ARGS=
CUTTER_WRAPPER=
if test x"$CUTTER_DEBUG" = x"yes"; then
    CUTTER_WRAPPER="$abs_top_builddir/libtool --mode=execute gdb --args"
    CUTTER_ARGS="--keep-opening-modules"
elif test x"$CUTTER_CHECK_LEAK" = x"yes"; then
    CUTTER_WRAPPER="$abs_top_builddir/libtool --mode=execute valgrind "
    CUTTER_WRAPPER="$CUTTER_WRAPPER --leak-check=full --show-reachable=yes -v"
    CUTTER_ARGS="--keep-opening-modules"
fi

export CUTTER

CUTTER_ARGS="$CUTTER_ARGS -s $BASE_DIR --exclude-directory fixtures"
if echo "$@" | grep -- --mode=analyze > /dev/null; then
    :
else
    CUTTER_ARGS="$CUTTER_ARGS --stream=xml --stream-log-directory $abs_top_builddir/log"
fi
if test x"$USE_GTK" = x"yes"; then
    CUTTER_ARGS="-u gtk $CUTTER_ARGS"
fi

if test -z "$RUBY"; then
    RUBY="$(${MAKE} -s -C "$abs_top_builddir" echo-ruby)"
fi

export RUBY

ruby_dir=$abs_top_builddir/binding/ruby
ruby_srcdir=$abs_top_srcdir/binding/ruby
MILTER_MANAGER_RUBYLIB=$ruby_srcdir/lib
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_dir/ext/core/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_dir/ext/client/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_dir/ext/server/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_dir/ext/manager/.libs
ruby_glib2_lib_dir=
ruby_glib2_ext_dir=
for dir in $(for dir in $ruby_dir/glib2-*; do echo $dir; done | sort -r); do
    if [ -f $dir/ext/glib2/glib2.so ]; then
	ruby_glib2_lib_dir=$ruby_srcdir/$(basename $dir)/lib
	ruby_glib2_ext_dir=$dir/ext/glib2
	break
    elif [ -f $dir/src/glib2.so ]; then
	ruby_glib2_lib_dir=$ruby_srcdir/$(basename $dir)/src/lib
	ruby_glib2_ext_dir=$dir/src
	break
    fi
done
if [ "$ruby_glib2_lib_dir" != "" ]; then
    MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_glib2_lib_dir
fi
if [ "$ruby_glib2_ext_dir" != "" ]; then
    MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_glib2_ext_dir
fi
RUBYLIB=$MILTER_MANAGER_RUBYLIB:$RUBYLIB
export MILTER_MANAGER_RUBYLIB
export RUBYLIB
export MILTER_MANAGER_CONFIGURATION_MODULE_DIR=$abs_top_builddir/module/configuration/ruby/.libs
export MILTER_MANAGER_CONFIG_DIR=$abs_top_srcdir/test/fixtures/configuration

$CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR
