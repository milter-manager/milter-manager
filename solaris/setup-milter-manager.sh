#! /usr/bin/bash --noprofile

set -e

PREFIX=$HOME/opt
PATH=$PREFIX/bin:/opt/csw/gnu:/opt/csw/bin:$PATH

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export ACLOCAL_OPTIONS="-I $PREFIX/share/aclocal/"

source ./functions.sh

base_dir=$(dirname $0)

install_milter_manager()
{
    echo -n "Configuring milter-manager..."
    (
        cd "${base_dir}/../"
        run bash ./autogen.sh
        run ./configure --prefix $PREFIX --enable-ruby-milter --with-package-platform=solaris
    )
    echo done.

    echo -n "Building milter-manager..."
    run ${MAKE} -C "${base_dir}/../"
    echo done.

    echo -n "Installing milter-manager..."
    run touch /tmp/timestamp
    run ${MAKE} -C "${base_dir}/../" install
    find $PREFIX -newer /tmp/timestamp -print | pkgproto | sed -e "s%$PREFIX/%%" -e "s/okimoto other/root root/"> "/tmp/prototype-milter-manager"
    echo done.
}

install_milter_manager

echo done.
