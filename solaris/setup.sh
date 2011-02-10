#!/usr/bin/bash --noprofile

set -e

PREFIX=$HOME/opt
PATH=$PREFIX/bin:/opt/csw/bin:$PATH

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH

source ./functions.sh

base_dir=$(dirname $0)
base_packages=$(cat "$base_dir/base-packages.list")
SOURCES="${base_dir}/sources"

source ./development-sourcelist

echo done.
