#! /usr/bin/bash --noprofile

set -e

PREFIX=$HOME/opt
PATH=$PREFIX/bin:/opt/csw/gnu:/opt/csw/bin:$PATH

export AR=/usr/ccs/bin/ar
export MAKE="/usr/sfw/bin/gmake -j4"
export CC="/usr/sfw/bin/gcc -m64"
export CFLAGS=-m64
export CXXFLAGS=-m64
export CPPFLAGS="-I$PREFIX/include"
export LDFLAGS="-L$PREFIX/lib"
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export ACLOCAL_OPTIONS="-I $PREFIX/share/aclocal/"

source ./functions.sh

base_dir=$(cd $(dirname $0); pwd)
log="#{base_dir}/milter-manager.build.log"
time_stamp_file="#{base_dir}/milter-manager.time_stamp"

install_milter_manager()
{
    echo "$(time_stamp): Configuring milter-manager..."
    (
        cd "${base_dir}/../"
        run bash ./autogen.sh
        run ./configure --prefix $PREFIX \
	    --enable-ruby-milter \
	    --with-package-platform=solaris
    ) > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building milter-manager..."
    run ${MAKE} -C "${base_dir}/../" > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Installing milter-manager..."
    run touch "${time_stamp_file}"
    run ${MAKE} -C "${base_dir}/../" install > "${log}"
    find $PREFIX -newer "${time_stamp_file}" -print | \
	pkgproto | \
	sed -e "s%$PREFIX/%%" -e "s/$USER other/root root/" \
	> "${base_dir}/milter-manager.prototype"
    echo "$(time_stamp): done."
}

install_milter_manager

echo done.
