#!/bin/sh
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

script_base_dir=`dirname $0`

if [ $# != 2 ]; then
    echo "Usage: $0 GPG_UID GPG_KEY_NAME DISTRIBUTIONS"
    echo " e.g.: $0 mitler-manager 'fedora centos'"
    exit 1
fi

GPG_KEY_NAME=$1
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
    for dir in $script_base_dir/${distribution}/*/*; do
        case $dir in
            *centos/5/*)
                echo $dir
                test -d $dir && run createrepo --checksum sha $dir
                ;;
            *)
                test -d $dir && run createrepo $dir
                ;;
        esac
    done;

    run cp $script_base_dir/RPM-GPG-KEY-${GPG_KEY_NAME} \
        $script_base_dir/${distribution}/RPM-GPG-KEY-${GPG_KEY_NAME};
done
