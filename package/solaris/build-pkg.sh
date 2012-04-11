#!/bin/bash

set -e

LANG=C

source ./environment.sh
source ./functions.sh

skip_milter_manager=$1
build_mysql=$2
build_ruby=$3

mkdir -p $PKGS
if test $skip_milter_manager = true; then
    if test $build_mysql = true; then
        echo "$(time_stamp): Building MySQL device..."
        pkgtrans -s $PKGS \
        "${PKG_PREFIX}mysql.pkg" \
        "${PKG_PREFIX}mysql"
        echo "$(time_stamp): done."
    fi
    if test $build_ruby = true; then
        echo "$(time_stamp): Building Ruby device..."
        pkgtrans -s $PKGS \
        "${PKG_PREFIX}ruby.pkg" \
        "${PKG_PREFIX}ruby"
        echo "$(time_stamp): done."
    fi
else
    echo "$(time_stamp): Building milter manager device..."
    pkgtrans -s $PKGS \
        "${PKG_PREFIX}milter-manager.pkg" \
        "${PKG_PREFIX}ruby" \
        "${PKG_PREFIX}iconv" \
        "${PKG_PREFIX}gettext" \
        "${PKG_PREFIX}glib" \
        "${PKG_PREFIX}mysql" \
        "${PKG_PREFIX}milter-manager"
    echo "$(time_stamp): done."
fi
