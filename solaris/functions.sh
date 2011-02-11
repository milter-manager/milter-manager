# -*- sh -*-

run()
{
    $@
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

time_stamp()
{
    date +%Y-%m-%dT%H:%m:%S
}

install_package()
{
    local url="$1"
    local tarball="${1##*/}"
    local base="${tarball%.tar.*}"
    local log="${BUILDS}/${base}.log"
    local time_stamp_file="${BUILDS}/${base}.time_stamp"
    shift

    mkdir -p "${BUILDS}"

    echo "$(time_stamp): Downloading ${base}..."
    run wget -N -P "${SOURCES}" "$url" > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Extracting ${base}..."
    run gtar xf "${SOURCES}/${tarball}" -C "${BUILDS}" > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Configuring ${base}..."
    (
        cd "${BUILDS}/${base}"
        run ./configure --enable-shared --prefix="${PREFIX}" "$@"
    ) > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building ${base}..."
    run touch "${time_stamp_file}"
    run ${MAKE} -C "${BUILDS}/${base}" > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Installing ${base}..."
    run ${MAKE} -C "${BUILDS}/${base}" install > "${log}"
    find $PREFIX -newer "${time_stamp_file}" -print | \
	pkgproto | \
	sed -e "s%$PREFIX/%%" -e "s/$USER other/root root/" > \
	"${BUILDS}/${base}.prototype"
    echo "$(time_stamp): done."
}

build_pkg()
{
    local pkg_dir="${base_dir}/${1}"
    local pkg_name="${PKG_PREFIX}${1}"
    local archive_name="${pkg_name}.pkg"
    local log="${PKG_BASE_DIR}/${pkg_name}.log"

    (
        cd $pkg_dir
        run pkgmk -o -r $PREFIX -a $(uname -p) -d $PKG_BASE_DIR
    ) > "${log}"
}

