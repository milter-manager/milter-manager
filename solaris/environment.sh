base_dir="$(cd $(dirname $0); pwd)"
SOURCES="${base_dir}/sources"
PATCHES="${base_dir}/patches"
BUILDS="${base_dir}/builds"
PROTOTYPES="${base_dir}/prototypes"
PKGS="${base_dir}/pkgs"

PREFIX=/opt/milter-manager
PATH=$PREFIX/bin:/opt/csw/bin:/usr/ccs/bin:/usr/sfw/bin:$PATH

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
COMPILER="gcc" # force to GCC. Solaris Studio isn't supported.
export COMPILER

if test "$COMPILER" = "solaris-studio"; then
    export CC=$SOLARIS_STUDIO_PREFIX/bin/cc
else
    export CC=/usr/sfw/bin/gcc
fi

if test -z "$ARCHITECTURE"; then
    ARCHITECTURE=amd64
fi
export ARCHITECTURE

if test "$ARCHITECTURE" = "amd64"; then
    CC="$CC -m64"
    export CFLAGS=-m64
    export CXXFLAGS=-m64
    LDFLAGS="-R/usr/lib/amd64 $LDFLAGS"
    LD_LIBRARY_PATH=/usr/lib/amd64:$LD_LIBRARY_PATH
else
    if test "$USE_OPEN_CSW" = "yes"; then
	LDFLAGS="-R/opt/csw/lib $LDFLAGS"
	LD_LIBRARY_PATH=/opt/csw/lib:$LD_LIBRARY_PATH
	PKG_CONFIG_PATH=/opt/csw/lib/pkgconfig:$PKG_CONFIG_PATH
	export XGETTEXT=/opt/csw/bin/gxgettext
	export MSGMERGE=/opt/csw/bin/gmsgmerge
	export MSGFMT=/opt/csw/bin/gmsgfmt
    fi
fi

export MAKE="/usr/sfw/bin/gmake"
export CPPFLAGS="-I$PREFIX/include"
export LDFLAGS="-L$PREFIX/lib -R$PREFIX/lib $LDFLAGS"
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH
export ACLOCAL_OPTIONS="-I $PREFIX/share/aclocal/"

export NROFF=/opt/csw/bin/groff
