#!/usr/bin/bash --noprofile

export PATH=/opt/csw/bin:/usr/sfw/bin:$PATH

source ./functions.sh

if test ! -x /opt/csw/bin/pkgutil; then
    yes | run pkgadd -d http://get.opencsw.org/now all
fi

run pkgutil -U

base_dir=$(dirname $0)

while read package; do
    if echo "$package" | grep -v '#' > /dev/null ; then
        run pkgutil -y -i "$package"
    fi
done < "$base_dir/base-packages.list"

