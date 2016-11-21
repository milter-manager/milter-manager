#!/bin/bash
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

if [ $# != 5 ]; then
    echo "Usage: $0 PACKAGE VERSION RELEASE ARCHITECTURES CODES"
    echo "Usage: $0 milter-manager 2.1.0 1 'i386 amd64' 'jessie stretch'"
    exit 1
fi

PACKAGE=$1
VERSION=$2
RELEASE=$3
ARCHITECTURES=$4
CODES=$5

script_base_dir=$(dirname $(realpath -e $0))

run()
{
    "$@"
    if test $? -ne 0; then
        echo "Failed $@"
        exit 1
    fi
}

upload_packages()
{
    architecture=$1
    code_name=$2

    case ${code_name} in
        jessie|stretch|unstable)
            distribution=debian
            component=main
            ;;
    esac
    if test ${code_name} = "unstable"; then
        return
    fi
    status=stable
    package_initial=$(echo ${PACKAGE} | sed -e 's/\(.\).*/\1/')
    pool_dir=${script_base_dir}/${distribution}/${status}/pool
    pool_dir=${pool_dir}/${code_name}/${component}/${package_initial}/${PACKAGE}

    for package in $(sed -ne "/^Files:/,$ p" $pool_dir/${PACKAGE}_${VERSION}-${RELEASE}_${architecture}.changes | grep -E "(\.deb|\.dsc)$" | cut -d" " -f6); do
        if test -f $pool_dir/$package; then
            package_cloud push $PACKAGE/repos/${distribution}/${code_name} $pool_dir/$package
        else
            echo "Missing: ${package}"
        fi
    done
}

for architecture in $ARCHITECTURES; do
    for code_name in $CODES; do
        upload_packages $architecture $code_name
    done
done
