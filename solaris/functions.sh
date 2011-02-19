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

update_prototype()
{
    local base="$1"
    local build_dir="$2"
    local package_name="${base%%-[0-9]*}"
    package_name="${package_name##lib}"
    local prototype_dir="${PROTOTYPES}/${package_name}"
    local prototype="${prototype_dir}/prototype"
    local user="$(/usr/xpg4/bin/id -un)"
    local group="$(/usr/xpg4/bin/id -gn)"
    local dest_dir="${BUILDS}/tmp"
    local log="${BUILDS}/${base}.log"

    echo "$(time_stamp): Updating prototype of ${base}..."
    run mkdir -p "$prototype_dir"
    run rm -rf "${dest_dir}"
    run ${MAKE} DESTDIR="${dest_dir}" -C "${build_dir}" install > "${log}"
    cat <<EOP > "${prototype}"
i pkginfo
i depend
i copyright
EOP
    find "${dest_dir}${PREFIX}" -print | \
	pkgproto | \
	grep -v " ${dest_dir}${PREFIX} " | \
	gsed -e "s%${dest_dir}${PREFIX}/%%" | \
	gsed -e "s/$user $group\$/root root/" | \
	sort >> "${prototype}"
    run rm -rf "${dest_dir}"
    echo "$(time_stamp): done."
}

install_package()
{
    local url="$1"
    local tar_ball="${1##*/}"
    local base="${tar_ball%.tar.*}"
    local build_dir="${BUILDS}/${base}"
    local log="${BUILDS}/${base}.log"
    shift

    mkdir -p "${BUILDS}"

    echo "$(time_stamp): Downloading ${base}..."
    if test ! -f "${SOURCES}/${tar_ball}"; then
	run wget -N -P "${SOURCES}" "$url" > "${log}"
    fi
    echo "$(time_stamp): done."

    echo "$(time_stamp): Extracting ${base}..."
    run rm -rf "${BUILDS}/${base}"
    run gtar xf "${SOURCES}/${tar_ball}" -C "${BUILDS}" > "${log}"
    echo "$(time_stamp): done."

    if test -d "${PATCHES}/${base}"; then
	for patch_file in $(echo "${PATCHES}/${base}/*" | sort); do
	    echo "$(time_stamp): Patching ${base} (${patch_file})..."
	    (
		cd "${BUILDS}/${base}"
		run patch -p1 < ${patch_file}
	    ) > "${log}"
	    echo "$(time_stamp): done."
	done
    fi

    echo "$(time_stamp): Configuring ${base}..."
    (
        cd "${BUILDS}/${base}"
	if test -f "CMakeLists.txt"; then
	    run cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" "$@" .
	else
            run ./configure --enable-shared --prefix="${PREFIX}" "$@"
	fi
    ) > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building ${base}..."
    run ${MAKE} -C "${build_dir}" > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Installing ${base}..."
    run ${MAKE} -C "${build_dir}" install > "${log}"
    echo "$(time_stamp): done."

    update_prototype "${base}" "${build_dir}"
}

build_pkg()
{
    local pkg_dir="${PROTOTYPES}/${1}"
    local pkg_name="${PKG_PREFIX}${1}"
    local archive_name="${pkg_name}.pkg"
    local log="${PKGS}/${pkg_name}.log"

    echo "$(time_stamp): Building package ${pkg_name}..."
    (
        cd $pkg_dir
        run pkgmk -o -r $PREFIX -d $PKGS
    ) > "${log}" 2>&1
    echo "$(time_stamp): done."
}

