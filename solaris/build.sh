#!/usr/bin/bash --noprofile

source ./functions.sh

skip_libraries=false

until test $# = 0; do
    case "$1" in
        (--skip-libraries)
            skip_libraries=true
            ;;
        (*)
            echo "invalid option: ${1}"
            ;;
    esac
    shift
done

if test $skip_libraries = true; then
    check_pkgs
    echo "skip install libraries."
else
    run ./install-libraries.sh
fi
run ./install-milter-manager.sh
run ./build-pkg.sh
