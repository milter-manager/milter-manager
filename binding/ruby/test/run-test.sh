#!/bin/bash
#
# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

if test -z "$TOP_SRCDIR"; then
    TOP_SRCDIR="$(${MAKE} -s -C "$abs_top_builddir" echo-abs-top-srcdir)"
fi
export TOP_SRCDIR

if test x"$NO_MAKE" != x"yes"; then
    $MAKE -C "$abs_top_builddir" > /dev/null || exit 1
fi

if test -z "$RUBY"; then
    RUBY="$(${MAKE} -s -C "$abs_top_builddir" echo-ruby)"
fi

if test -z "$MILTER_TEST_CLIENT"; then
    MILTER_TEST_CLIENT="${abs_top_builddir}/tool/milter-test-client"
fi
export MILTER_TEST_CLIENT

if test -z "$MILTER_TEST_SERVER"; then
    MILTER_TEST_SERVER="${abs_top_builddir}/tool/milter-test-server"
fi
export MILTER_TEST_SERVER

if test -z "$TEST_UNIT_MAX_DIFF_TARGET_STRING_SIZE"; then
    TEST_UNIT_MAX_DIFF_TARGET_STRING_SIZE=5000
    export TEST_UNIT_MAX_DIFF_TARGET_STRING_SIZE
fi

RUBY_WRAPPER=
if test x"$RUBY_DEBUG" = x"yes"; then
    RUBY_WRAPPER="gdb --args"
fi

ruby_build_dir=$abs_top_builddir/binding/ruby
ruby_source_dir=$TOP_SRCDIR/binding/ruby
MILTER_MANAGER_RUBYLIB=$ruby_source_dir/lib
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_build_dir/ext/core/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_build_dir/ext/client/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_build_dir/ext/server/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_build_dir/ext/manager/.libs
ruby_glib2_lib_dir=
ruby_glib2_ext_dir=
for dir in $(for dir in $ruby_build_dir/glib2-*; do echo $dir; done | sort -r); do
    if [ -f $dir/ext/glib2/glib2.so ]; then
	ruby_glib2_lib_dir=$ruby_source_dir/$(basename $dir)/lib
	ruby_glib2_ext_dir=$dir/ext/glib2
	break
    elif [ -f $dir/src/glib2.so ]; then
	ruby_glib2_lib_dir=$ruby_source_dir/$(basename $dir)/src/lib
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

export MILTER_MANAGER_CONFIGURATION_MODULE_DIR=$abs_top_builddir/module/configuration/ruby/.libs

$RUBY_WRAPPER $RUBY -I "$MILTER_MANAGER_RUBYLIB" \
    "$ruby_source_dir/test/run-test.rb" "$@"
