#!/usr/bin/bash --noprofile

export PATH=/opt/csw/bin:/usr/sfw/bin:$PATH

source ./functions

if test ! -x /opt/csw/bin/pkg-get; then
    run pkgadd -d http://mirror.opencsw.org/opencsw/pkg_get.pkg
fi
run pkg-get install gnupg
run wget -O /tmp/gpg.key http://www.opencsw.org/get-it/mirrors/
run gpg --import /tmp/gpg.key

run pkg-get -U

run pkg-get install "$base_packages"

