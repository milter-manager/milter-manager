#!/bin/bash

script_base_dir=`dirname $0`

if [ $# != 1 ]; then
    echo "Usage: $0 DISTRIBUTIONS"
    echo " e.g.: $0 'fedora centos'"
    exit 1
fi

DISTRIBUTIONS=$1

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for distribution in ${DISTRIBUTIONS}; do
    packages=()
    for package in `find $script_base_dir/${distribution}/*/*/*/ -name \*.rpm -print`; do
	if ! rpm -Kv $package | grep -q -i signature; then
	    packages=("${packages[@]}" "${package}")
	fi
    done
    run rpm -D "_gpg_name `$script_base_dir/gpg-uid.sh`" \
	--addsign `echo -n "${packages[@]}"`
done
