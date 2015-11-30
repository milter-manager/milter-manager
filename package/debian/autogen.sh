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

# for old intltoolize
if [ ! -d config/po ]; then
    mkdir -p config
    ln -s ../po config/po
fi

if [ x"$1" = x--no-update ]; then
    shift
    update=no
fi

run ${ACLOCAL:-aclocal} $ACLOCAL_OPTIONS
run ${LIBTOOLIZE:-libtoolize} --copy --force
run ${INTLTOOLIZE:-intltoolize} --force --copy
run ${GTKDOCIZE:-gtkdocize} --copy
run ${AUTOHEADER:-autoheader}
run ${AUTOMAKE:-automake} --add-missing --foreign --copy
run ${AUTOCONF:-autoconf}
