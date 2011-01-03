#!/bin/sh
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

if gmake --version > /dev/null 2>&1; then
    MAKE=${MAKE:-"gmake"}
else
    MAKE=${MAKE:-"make"}
fi

if test -z "$abs_top_builddir"; then
    abs_top_builddir="$(${MAKE} -s echo-abs-top-builddir)"
fi

if test -z "$abs_top_srcdir"; then
    abs_top_srcdir="$(${MAKE} -C "$abs_top_builddir" -s echo-abs-top-srcdir)"
fi

if test x"$NO_MAKE" != x"yes"; then
    $MAKE -C "$abs_top_builddir" > /dev/null || exit 1
fi

if test -z "$RUBY"; then
    RUBY="$(${MAKE} -s -C "$abs_top_builddir" echo-ruby)"
fi

RUBY_WRAPPER=
if test x"$RUBY_DEBUG" = x"yes"; then
    CUTTER_WRAPPER="gdb --args"
fi

ruby_build_dir=$abs_top_builddir/binding/ruby
ruby_source_dir=$abs_top_srcdir/binding/ruby
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_source_dir/lib
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_build_dir/src/toolkit/.libs
MILTER_MANAGER_RUBYLIB=$MILTER_MANAGER_RUBYLIB:$ruby_build_dir/src/manager/.libs
ruby_glib2_lib_dir=
ruby_glib2_ext_dir=
for dir in $(for dir in $ruby_build_dir/glib-*; do echo $dir; done | sort -r); do
    if [ -f $dir/ext/glib2/glib2.so ]; then
	ruby_glib2_lib_dir=$dir/lib
	ruby_glib2_ext_dir=$dir/ext/glib2
	break
    elif [ -f $dir/src/glib2.so ]; then
	ruby_glib2_lib_dir=$dir/src/lib
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

$RUBY_WRAPPER $RUBY "$ruby_source_dir/test/run-test.rb" "$@"
