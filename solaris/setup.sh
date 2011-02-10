#!/usr/bin/bash --noprofile

set -e

PREFIX=$HOME/opt
PATH=$PREFIX/bin:/opt/csw/bin:/usr/sfw/bin:$PATH

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH

source ./functions.sh

base_dir=$(dirname $0)
SOURCES="${base_dir}/sources"
BUILDS="${base_dir}/builds"

source ./development-sourcelist

echo done.
