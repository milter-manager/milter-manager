#! /usr/bin/bash --noprofile

set -e

PREFIX=$HOME/opt
PATH=$PREFIX/bin:/opt/csw/bin:$PATH

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

source ./functions.sh

base_dir=$(dirname $0)

install_milter_manager

echo done.
