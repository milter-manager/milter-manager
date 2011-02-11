#!/bin/bash

set -e

LANG=C

base_dir=$(cd $(dirname $0); pwd)
PREFIX=$HOME/opt
PKG_BASE_DIR="${base_dir}/pkgs"
PKG_PREFIX=MMGR

source ./functions.sh

mkdir -p $PKG_BASE_DIR

echo "$(time_stamp): Building packages..."
build_pkg ruby
build_pkg iconv
build_pkg gettext
build_pkg glib
build_pkg milter-manager
echo "$(time_stamp): done."

echo "$(time_stamp): Building milter manager device..."
pkgtrans -s $PKG_BASE_DIR \
    "${PKG_PREFIX}milter-manager.pkg" \
    "${PKG_PREFIX}ruby" \
    "${PKG_PREFIX}iconv" \
    "${PKG_PREFIX}gettext" \
    "${PKG_PREFIX}glib" \
    "${PKG_PREFIX}milter-manager"
echo "$(time_stamp): done."
