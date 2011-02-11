PREFIX=$HOME/opt
PATH=$PREFIX/bin:/opt/csw/gnu:/opt/csw/bin:$PATH

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export CPPFLAGS="-I$PREFIX/include"
export LDFLAGS="-L$PREFIX/lib"
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export ACLOCAL_OPTIONS="-I $PREFIX/share/aclocal/"

