#!/usr/bin/bash --noprofile

source ./environment.sh
source ./functions.sh

skip_libraries=true

until test $# = 0; do
    case "$1" in
        (--all)
            skip_libraries=false
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
