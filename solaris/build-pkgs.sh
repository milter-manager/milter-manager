#!/bin/bash

set -e

LANG=C

PKG_PREFIX=MMGR

source ./environment.sh
source ./functions.sh

mkdir -p $PKGS

echo "$(time_stamp): Building packages..."
build_pkg ruby
build_pkg iconv
build_pkg gettext
build_pkg glib
build_pkg mysql
build_pkg milter-manager
echo "$(time_stamp): done."

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
