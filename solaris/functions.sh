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
    local package_name="${base%%-*}"
    local log="${BUILDS}/${base}.log"
    local time_stamp_file="${BUILDS}/${base}.time_stamp"
    local prototype="${PROTOTYPES}/${package_name}/prototype"
    local user="$(/usr/xpg4/bin/id -un)"
    local group="$(/usr/xpg4/bin/id -gn)"
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
    cat <<EOP > "${prototype}"
i pkginfo
i depend
i copyright
EOP
    find $PREFIX -newer "${time_stamp_file}" -print | \
	pkgproto | \
	grep -v " $PREFIX " | \
	gsed -e "s%$PREFIX/%%" | \
	gsed -e "s/$user $group$/root root/" | \
	sort >> "${prototype}"
    echo "$(time_stamp): done."
}

build_pkg()
{
    local pkg_dir="${base_dir}/${1}"
    local pkg_name="${PKG_PREFIX}${1}"
    local archive_name="${pkg_name}.pkg"
    local log="${PKG_BASE_DIR}/${pkg_name}.log"

    echo "$(time_stamp): Building package ${pkg_name}..."
    (
        cd $pkg_dir
        run pkgmk -o -r $PREFIX -d $PKG_BASE_DIR
    ) > "${log}" 2>&1
    echo "$(time_stamp): done."
}

