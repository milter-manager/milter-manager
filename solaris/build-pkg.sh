#!/bin/bash

set -e

LANG=C

source ./environment.sh
source ./functions.sh

mkdir -p $PKGS

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
