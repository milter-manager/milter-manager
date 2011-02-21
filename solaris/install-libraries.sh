#!/usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

echo "$(time_stamp): Installing libraries..."

gnu_org="http://ftp.gnu.org/pub/gnu"
gnome_org="http://ftp.gnome.org/pub/gnome/sources"
ruby_lang_org="ftp://ftp.ruby-lang.org/pub/ruby"
mysql_downloads="http://ftp.jaist.ac.jp/pub/mysql/Downloads"

if test "$USE_OPEN_CSW" != "yes"; then
    CC="$CC -m64" \
	install_package ${gnu_org}/gettext/gettext-0.18.1.1.tar.gz \
	--enable-relocatable --without-git
    install_package ${gnu_org}/libiconv/libiconv-1.13.1.tar.gz
    CC="$CC -m64" \
	install_package ${gnu_org}/gettext/gettext-0.18.1.1.tar.gz \
	--enable-relocatable --without-git
    LDFLAGS="-lsocket -lnsl $LDFLAGS" \
	install_package ${gnome_org}/glib/2.28/glib-2.28.1.tar.bz2 \
	--with-libiconv=gnu
fi

CC="$CC -m64" \
    install_package ${mysql_downloads}/MySQL-5.5/mysql-5.5.9.tar.gz

CC="$CC -m64" \
    install_package ${ruby_lang_org}/1.9/ruby-1.9.2-p180.tar.bz2 \
    --disable-install-doc --enable-shared --enable-load-relative

gem_prefix=${PROTOTYPES_DESTDIR}${PREFIX}/lib/ruby/gems/1.9.1
gem_options="--no-ri --no-rdoc"
gem_install="gem install ${gem_options} bundler mysql2 activerecord mail"
run $gem_install
CC="$CC -m64" \
    update_prototype ruby-1.9.2-p180 \
    "${gem_install} --install-dir ${gem_prefix}"

install_package http://downloads.sourceforge.net/cutter/cutter-1.1.7.tar.gz

echo "$(time_stamp): done."
