#! /usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

install_milter_manager()
{
    local base="milter-manager"
    local log="${BUILDS}/${base}.build.log"
    local build_dir="${base_dir}/../"

    if test -f "${build_dir}/Makefile"; then
	echo "$(time_stamp): Cleaning ${base}..."
	run ${MAKE} -C "${build_dir}" clean > "${log}"
	echo "$(time_stamp): done."
    fi

    echo "$(time_stamp): Configuring ${base}..."
    (
        cd "${build_dir}"
        run bash ./autogen.sh
        run ./configure --prefix $PREFIX \
	    --enable-ruby-milter \
	    --with-package-platform=solaris
    ) > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building ${base}..."
    run ${MAKE} -C "${build_dir}" > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Installing ${base}..."
    run ${MAKE} -C "${build_dir}" install > "${log}"
    echo "$(time_stamp): done."

    update_prototype "milter-manager" "${build_dir}"
}

echo "$(time_stamp): Installing milter manager..."
install_milter_manager
echo "$(time_stamp): done."
