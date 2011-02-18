base_dir="$(cd $(dirname $0); pwd)"
SOURCES="${base_dir}/sources"
BUILDS="${base_dir}/builds"
PROTOTYPES="${base_dir}/prototypes"
PKGS="${base_dir}/pkgs"

PREFIX=/opt/milter-manager
PATH=$PREFIX/bin:/opt/csw/bin:/usr/sfw/bin:$PATH

if test -z "$SOLARIS_STUDIO_PREFIX"; then
    SOLARIS_STUDIO_PREFIX=$(echo /opt/solstudio* | sort -r | head -1)
fi

if test -z "$COMPILER"; then
    if test -n "$SOLARIS_STUDIO_PREFIX" -a \
	-x "$SOLARIS_STUDIO_PREFIX/bin/cc"; then
	COMPILER="solaris-studio"
    else
	COMPILER="gcc"
    fi
fi

if test "$COMPILER" = "solaris-studio"; then
    export CC=$SOLARIS_STUDIO_PREFIX/bin/cc
    PATH=$SOLARIS_STUDIO_PREFIX/bin:$PATH
else
    export AR=/usr/ccs/bin/ar
    export CC=/usr/sfw/bin/gcc
fi

if test -z "$ARCHITECTURE"; then
    ARCHITECTURE=32
fi

if test "$ARCHITECTURE" = 64; then
    CC="$CC -m64"
    export CFLAGS=-m64
    export CXXFLAGS=-m64
fi

export MAKE="/usr/sfw/bin/gmake -j4"
export CPPFLAGS="-I$PREFIX/include"
export LDFLAGS="-L$PREFIX/lib"
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export ACLOCAL_OPTIONS="-I $PREFIX/share/aclocal/"
