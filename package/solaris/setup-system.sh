#!/usr/bin/bash --noprofile

export PATH=/opt/csw/bin:/usr/sfw/bin:$PATH

source ./functions.sh

if test ! -x /opt/csw/bin/pkgutil; then
    yes | run pkgadd -d http://get.opencsw.org/now
fi

run pkgutil -U

base_dir=$(dirname $0)
base_packages=$(grep -v '#' "$base_dir/base-packages.list")

run pkgutil -y -i "$base_packages"

