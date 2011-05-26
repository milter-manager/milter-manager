#! /usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

install_milter_manager()
{
    local base="milter-manager"
    local log="${BUILDS}/${base}.build.log"
    local build_dir="${base_dir}/../"

    mkdir -p "${BUILDS}"

    if test -f "${build_dir}/Makefile"; then
	echo "$(time_stamp): Cleaning ${base}..."
	run ${MAKE} -C "${build_dir}" clean > "${log}"
	echo "$(time_stamp): done."
    fi

    echo "$(time_stamp): Configuring ${base}..."
    (
        cd "${build_dir}"
        ACLOCAL_OPTIONS="-I ${PREFIX}/share/aclocal" \
	    run bash ./autogen.sh --no-update
        run ./configure CFLAGS="$CFLAGS -ggdb3" \
          --prefix $PREFIX --enable-ruby-milter "$@"
    ) > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building ${base}..."
    run ${MAKE} -C "${build_dir}" > "${log}"
    echo "$(time_stamp): done."

    build_pkg "milter-manager" "${build_dir}"
    install_pkg "milter-manager"
}

echo "$(time_stamp): Installing milter manager package..."
if test -z "${LOCALSTATEDIR}"; then
    install_milter_manager
else
    install_milter_manager --localstatedir=${LOCALSTATEDIR}
fi
echo "$(time_stamp): done."
