#!/bin/sh
#
# Copyright (C) 2008-2010  Kouhei Sutou <kou@claer-code.com>
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

unset LANG

export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/.."
top_dir="`cd $top_dir; pwd`"

if test x"$NO_MAKE" != x"yes"; then
    if which gmake > /dev/null; then
	MAKE=${MAKE:-"gmake"}
    else
	MAKE=${MAKE:-"make"}
    fi
    $MAKE -C $top_dir/ > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="`make -s -C $BASE_DIR echo-cutter`"
fi

CUTTER_ARGS=
CUTTER_WRAPPER=
if test x"$CUTTER_DEBUG" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute gdb --args"
    CUTTER_ARGS="--keep-opening-modules"
elif test x"$CUTTER_CHECK_LEAK" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute valgrind "
    CUTTER_WRAPPER="$CUTTER_WRAPPER --leak-check=full --show-reachable=yes -v"
    CUTTER_ARGS="--keep-opening-modules"
fi

export CUTTER

CUTTER_ARGS="$CUTTER_ARGS -s $BASE_DIR --exclude-directory fixtures"
if echo "$@" | grep -- --mode=analyze > /dev/null; then
    :
else
    CUTTER_ARGS="$CUTTER_ARGS --stream=xml --stream-log-directory $top_dir/log"
fi
if test x"$USE_GTK" = x"yes"; then
    CUTTER_ARGS="-u gtk $CUTTER_ARGS"
fi

ruby_dir=$top_dir/binding/ruby
RUBYLIB=$RUBYLIB:$ruby_dir/lib
RUBYLIB=$RUBYLIB:$ruby_dir/src/toolkit/.libs
RUBYLIB=$RUBYLIB:$ruby_dir/src/manager/.libs
ruby_glib2_dir=
if [ -f $ruby_dir/glib-0.19.3/src/glib2.so ]; then
    ruby_glib2_dir=$ruby_dir/glib-0.19.3
elif [ -f $ruby_dir/glib-0.16.0/src/glib2.so ]; then
    ruby_glib2_dir=$ruby_dir/glib-0.16.0
fi
if [ "$ruby_glib2_dir" != "" ]; then
    RUBYLIB=$RUBYLIB:$ruby_glib2_dir/src/lib
    RUBYLIB=$RUBYLIB:$ruby_glib2_dir/src
fi
export RUBYLIB
export MILTER_MANAGER_CONFIGURATION_MODULE_DIR=$top_dir/module/configuration/ruby/.libs
export MILTER_MANAGER_CONFIG_DIR=$top_dir/test/fixtures/configuration

$CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR
