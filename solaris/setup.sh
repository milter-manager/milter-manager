#!/usr/bin/bash

PATH=/usr/local/bin:/opt/csw/bin:/usr/sfw/bin:$PATH
PREFIX=$HOME/opt

AR=/usr/ccs/bin/ar
MAKE="/usr/sfw/bin/gmake -j4"
CC=/usr/sfw/bin/gcc
CFLAGS=-m64

test -f ./functions && source ./functions

base_dir=$(dirname $0)
base_packages=$(cat "$base_dir/base-packages.list")
SOURCES="${base_dir}/sources"

if test ! -x /opt/csw/bin/pkg-get; then
    run pkgadd -d http://mirror.opencsw.org/opencsw/pkg_get.pkg
fi
run pkg-get install gnupg
run wget -O /tmp/gpg.key http://www.opencsw.org/get-it/mirrors/
run gpg --import /tmp/gpg.key

run pkg-get -U

run pkg-get install "$base_packages"


echo done.
