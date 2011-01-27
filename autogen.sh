#!/bin/sh

run()
{
    $@
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

# for old intltoolize
if [ ! -d config/po ]; then
    ln -s ../po config/po
fi

cutter_repository=https://cutter.svn.sourceforge.net/svnroot/cutter/cutter
svn export --force ${cutter_repository}/trunk/misc

clear_code_repository=http://www.clear-code.com/repos/svn
svn export --force ${clear_code_repository}/tdiary html/blog/clear-code

run ${ACLOCAL:-aclocal} $ACLOCAL_OPTIONS
run ${LIBTOOLIZE:-libtoolize} --copy --force
run ${INTLTOOLIZE:-intltoolize} --force --copy
#run ${GTKDOCIZE:-gtkdocize} --copy
run ${AUTOHEADER:-autoheader}
run ${AUTOMAKE:-automake} --add-missing --foreign --copy
run ${AUTOCONF:-autoconf}
