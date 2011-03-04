#!/bin/sh

update=yes

run()
{
    $@
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

svn_update()
{
    local repo="$1" dir="${2-`basename \"$1\"`}"
    if [ $update != yes ]; then return; fi
    if test -d "$dir/.svn"; then
	svn update "$dir"
    else
	svn export --force "$repo" "$dir"
    fi
}

# for old intltoolize
if [ ! -d config/po ]; then
    ln -s ../po config/po
fi

if [ x"$1" = x--no-update ]; then
    shift
    update=no
fi

cutter_repository=https://cutter.svn.sourceforge.net/svnroot/cutter/cutter
run svn_update ${cutter_repository}/trunk/misc

clear_code_repository=http://www.clear-code.com/repos/svn
run svn_update ${clear_code_repository}/tdiary html/blog/clear-code

test_unit_repository=http://test-unit.rubyforge.org/svn
run svn_update ${test_unit_repository}/trunk binding/ruby/test-unit

run ${ACLOCAL:-aclocal} $ACLOCAL_OPTIONS
run ${LIBTOOLIZE:-libtoolize} --copy --force
run ${INTLTOOLIZE:-intltoolize} --force --copy
#run ${GTKDOCIZE:-gtkdocize} --copy
run ${AUTOHEADER:-autoheader}
run ${AUTOMAKE:-automake} --add-missing --foreign --copy
run ${AUTOCONF:-autoconf}
