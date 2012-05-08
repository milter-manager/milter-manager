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
    local repository="$1"
    local dir="${2-`basename $repository`}"
    if [ $update != yes -a -d "$dir" ]; then
	return
    fi
    if test -d "$dir/.svn"; then
	svn update "$dir"
    else
	svn export --force "$repository" "$dir"
    fi
}

git_update()
{
    local repository="$1"
    local dir="${2-`basename $repository`}"
    if [ $update != yes -a -d "$dir" ]; then
	return
    fi
    if test -d "$dir/.git"; then
	(cd "$dir" && git pull --rebase)
    else
	rm -rf "$dir"
	git clone "$repository" "$dir"
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

clear_code_repository=http://www.clear-code.com/repos/svn
run svn_update ${clear_code_repository}/tdiary html/blog/clear-code

test_unit_repository=https://github.com/test-unit/test-unit.git
run git_update ${test_unit_repository} binding/ruby/test-unit

run ${ACLOCAL:-aclocal} $ACLOCAL_OPTIONS
run ${LIBTOOLIZE:-libtoolize} --copy --force
run ${INTLTOOLIZE:-intltoolize} --force --copy
#run ${GTKDOCIZE:-gtkdocize} --copy
run ${AUTOHEADER:-autoheader}
run ${AUTOMAKE:-automake} --add-missing --foreign --copy
run ${AUTOCONF:-autoconf}
