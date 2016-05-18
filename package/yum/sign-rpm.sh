#!/bin/bash
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

script_base_dir=`dirname $0`

if [ $# != 2 ]; then
    echo "Usage: $0 GPG_UID DISTRIBUTIONS"
    echo " e.g.: $0 1BD22CD1 'fedora centos'"
    exit 1
fi

GPG_UID=$1
DISTRIBUTIONS=$2

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
    for package in `find $script_base_dir/${distribution}/[67]/*/*/ -name \*.rpm -print`; do
        if ! rpm -Kv $package | grep -q -i signature; then
            packages=("${packages[@]}" "${package}")
        fi
    done
    run rpm -D "_gpg_name ${GPG_UID}" \
        --addsign `echo -n "${packages[@]}"`
done
