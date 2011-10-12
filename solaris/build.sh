#!/usr/bin/bash --noprofile

source ./environment.sh
source ./functions.sh

skip_libraries=true
skip_milter_manager=false
build_other=false
build_mysql=false
build_ruby=false

until test $# = 0; do
    case "$1" in
        (--all)
            skip_libraries=false
            build_other=true
            build_mysql=true
            build_ruby=true
            ;;
        (--mysql)
            skip_libraries=false
            skip_milter_manager=true
            build_mysql=true
            ;;
        (--ruby)
            skip_libraries=false
            skip_milter_manager=true
            build_ruby=true
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
    run ./install-libraries.sh $build_other $build_mysql $build_ruby
fi
if test $skip_milter_manager = true; then
    echo "skip install milter-manager."
else
    run ./install-milter-manager.sh
    run ./build-pkg.sh $skip_milter_manager $build_mysql $build_ruby
fi
