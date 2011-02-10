#!/bin/bash

set -e

LANG=C

base_dir=$(cd $(dirname $0); pwd)
PREFIX=$HOME/opt
PKG_BASE_DIR="${base_dir}/pkgs"
PKG_PREFIX=MMGR

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export PKG_CONFIG_PATH=$BASE_PREFIX/lib/pkgconfig
export LD_LIBRARY_PATH=$BASE_PREFIX/lib:$LD_LIBRARY_PATH

source ./functions.sh

mkdir -p $PKG_BASE_DIR

build_pkg ruby
build_pkg iconv
build_pkg gettext
build_pkg glib
build_pkg milter-manager

pkgtrans -s $PKG_BASE_DIR MMGRmilter-manager.pkg MMGRruby MMGRiconv MMGRgettext MMGRglib MMGRmilter-manager
