#!/usr/bin/bash

PATH=/usr/local/bin:/opt/csw/bin:/usr/sfw/bin:$PATH

pkgadd -d http://mirror.opencsw.org/opencsw/pkg_get.pkg

pkg-get install sudo
