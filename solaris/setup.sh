#!/usr/bin/bash

PATH=/usr/local/bin:/opt/csw/bin:/usr/sfw/bin:$PATH
PREFIX=/opt/local

AR=/usr/ccs/bin/ar
MAKE="/usr/sfw/bin/gmake -j4"
CC=/usr/sfw/bin/gcc
CFLAGS=-m64

run()
{
    $@
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

base_dir=$(dirname $0)
base_packages=$(cat "$base_dir/base-packages.list")
SOURCES="${base_dir}/sources"

if test ! -x /opt/csw/bin/pkg-get; then
    run pkgadd -d http://mirror.opencsw.org/opencsw/pkg_get.pkg
fi
run pkg-get install gnupg
wget -O /tmp/gpg.key http://www.opencsw.org/get-it/mirrors/
gpg --import /tmp/gpg.key

mkdir -p /.gnupg
if ! grep search.keyserver.net /.gnupg/options; then
    echo keyserver search.keyserver.net >>/.gnupg/options
fi
if ! grep search.keyserver.net /.gnupg/gpg.conf; then
    echo keyserver search.keyserver.net >>/.gnupg/gpg.conf
fi
run pkg-get -U

run pkg-get install "$base_packages"

inst()
{
    url="$1"
    tarball="${1##*/}"
    base="${tarball%.tar.*}"
    build_dir="${PREFIX}/build"
    shift

    mkdir -p "$build_dir"


    echo -n "Downloading ${base}..."
    run wget -N -P "${SOURCES}" "$url"
    echo done.

    echo -n "Extracting ${base}..."
    run gtar xf "${SOURCES}/${tarball}" -C "${build_dir}"
    echo done.

    echo -n "Configuring ${base}..."
    (
        cd "${build_dir}/${base}"
        run ./configure --enable-shared --prefix="${PREFIX}" "$@"
    )
    echo done.

    echo -n "Building ${base}..."
    run ${MAKE} -C "${build_dir}/${base}"
    echo done.

    echo -n "Installing ${base}..."
    run ${MAKE} -C "${build_dir}/${base}" prefix="${PREFIX}" install
    echo done.
}

test -f ./development_sourcelist && source ./development_sourcelist

echo done.
