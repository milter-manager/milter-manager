#!/usr/bin/bash

PATH=/usr/local/bin:/opt/csw/bin:/usr/sfw/bin:$PATH

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

pkgadd -d http://mirror.opencsw.org/opencsw/pkg_get.pkg

pkg-get install $(base_pacakges)
