#!/usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

base_dir=$(dirname $0)
SOURCES="${base_dir}/sources"
BUILDS="${base_dir}/builds"

echo "$(time_stamp): Installing libraries..."

install_package ftp://ftp.ruby-lang.org/pub/ruby/1.9/ruby-1.9.2-p136.tar.bz2 \
    --disable-install-doc --enable-shared
install_package http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.1.1.tar.gz \
    --enable-relocatable --without-git
install_package http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.13.1.tar.gz
install_package http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.1.1.tar.gz \
    --enable-relocatable --without-git
install_package http://ftp.gnome.org/pub/gnome/sources/glib/2.22/glib-2.22.5.tar.bz2 \
    --with-libiconv=gnu

install_package http://downloads.sourceforge.net/cutter/cutter-1.1.6.tar.gz

echo "$(time_stamp): done."
