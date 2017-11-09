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
    mkdir -p config
    ln -s ../po config/po
fi

git submodule update --init

run mkdir -p m4

run ${INTLTOOLIZE:-intltoolize} --force --copy
run ${GTKDOCIZE:-gtkdocize} --copy
run ${LIBTOOLIZE:-libtoolize} --copy --force
run ${ACLOCAL:-aclocal} -I m4 $ACLOCAL_OPTIONS
run ${AUTOHEADER:-autoheader}
run ${AUTOMAKE:-automake} --add-missing --foreign --copy
run ${AUTOCONF:-autoconf}
# run ${AUTORECONF:-autoreconf} --force --install --verbose
