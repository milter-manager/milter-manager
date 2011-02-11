#! /usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

log="${BUILDS}/milter-manager.build.log"
time_stamp_file="${BUILDS}/milter-manager.time_stamp"

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
	> "${BUILDS}/milter-manager.prototype"
    echo "$(time_stamp): done."
}

echo "$(time_stamp): Installing milter manager..."
install_milter_manager
echo "$(time_stamp): done."
