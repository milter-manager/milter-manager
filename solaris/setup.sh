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

for package in "$base_packages"; do
    run pkg-get install $package
done


echo done.
