#!/usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

echo "$(time_stamp): Installing libraries..."

gnu_org="http://ftp.gnu.org/pub/gnu"
gnome_org="http://ftp.gnome.org/pub/gnome/sources"
mysql_downloads="http://ftp.jaist.ac.jp/pub/mysql/Downloads"

if test "$USE_CSW" != "yes"; then
    install_package ${gnu_org}/gettext/gettext-0.18.1.1.tar.gz \
	--enable-relocatable --without-git
    install_package ${gnu_org}/libiconv/libiconv-1.13.1.tar.gz
    install_package ${gnu_org}/gettext/gettext-0.18.1.1.tar.gz \
	--enable-relocatable --without-git
    if test "$ARCHITECTURE" = "i386"; then
	install_package ${gnome_org}/glib/2.24/glib-2.24.2.tar.bz2 \
	    --with-libiconv=gnu
    else
	install_package ${gnome_org}/glib/2.22/glib-2.22.5.tar.bz2 \
	    --with-libiconv=gnu
    fi
fi

install_package ${mysql_downloads}/MySQL-5.5/mysql-5.5.9.tar.gz

install_package ftp://ftp.ruby-lang.org/pub/ruby/1.9/ruby-1.9.2-p180.tar.bz2 \
    --disable-install-doc --enable-shared

install_package http://downloads.sourceforge.net/cutter/cutter-1.1.7.tar.gz

echo "$(time_stamp): done."
