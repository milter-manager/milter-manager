#!/usr/bin/bash --noprofile

set -e

PATH=/opt/csw/bin:/usr/sfw/bin:$PATH
PREFIX=$HOME/opt

AR=/usr/ccs/bin/ar
MAKE="/usr/sfw/bin/gmake -j4"
CC=/usr/sfw/bin/gcc
CFLAGS=-m64

test -f ./functions && source ./functions

base_dir=$(dirname $0)
base_packages=$(cat "$base_dir/base-packages.list")
SOURCES="${base_dir}/sources"

test -f ./development-sourcelist && source ./development-sourcelist

echo done.
