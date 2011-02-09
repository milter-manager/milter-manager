#!/usr/bin/bash --noprofile

set -e

export PATH=/opt/csw/bin:/usr/sfw/bin:$PATH
PREFIX=$HOME/opt

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -m64 -j4"
export CC=/usr/sfw/bin/gcc
export CFLAGS=-m64
export CXXFLAGS=-m64

source ./functions.sh

base_dir=$(dirname $0)
base_packages=$(cat "$base_dir/base-packages.list")
SOURCES="${base_dir}/sources"

source ./development-sourcelist

echo done.
