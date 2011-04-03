# -*- sh -*-

run()
{
    $@
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

run_sudo()
{
    run sudo "$@"
}

time_stamp()
{
    date +%Y-%m-%dT%H:%m:%S
}

build_pkg()
{
    local base="$1"
    local build_dir="$2"
    local package_name="${base%%-[0-9]*}"
    package_name="${package_name##lib}"
    local prototype_dir="${PROTOTYPES}/${package_name}"
    local prototype="${prototype_dir}/prototype"
    local pkginfo_template="${prototype_dir}/pkginfo.in"
    local pkginfo="${prototype_dir}/pkginfo"
    local preinstall="${prototype_dir}/preinstall"
    local postinstall="${prototype_dir}/postinstall"
    local user="$(/usr/xpg4/bin/id -un)"
    local group="$(/usr/xpg4/bin/id -gn)"
    local log="${BUILDS}/${base}.log"
    shift
    shift

    echo "$(time_stamp): Creating pkginfo of ${base}..."
    sed -e "s,@prefix@,${PREFIX},g" "${pkginfo_template}" > "${pkginfo}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Updating prototype of ${base}..."
    run mkdir -p "$prototype_dir"
    run rm -rf "${PKG_DESTDIR}"
    run ${MAKE} DESTDIR="${PKG_DESTDIR}" -C "${build_dir}" install > "${log}"
    for command in "$@"; do
	run $command > "${log}"
    done
    cat <<EOP > "${prototype}"
i pkginfo
i depend
i copyright
EOP
    test -f "${preinstall}" && echo "i preinstall" >> "${prototype}"
    test -f "${postinstall}" && echo "i postinstall" >> "${prototype}"
    find "${PKG_DESTDIR}${PREFIX}" -print | \
	pkgproto | \
	grep -v " ${PKG_DESTDIR}${PREFIX} " | \
	gsed -e "s%${PKG_DESTDIR}${PREFIX}/%%" | \
	gsed -e "s/$user $group\$/root root/" | \
	sort >> "${prototype}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building package ${pkg_name}..."
    (
        cd $pkg_dir
        run pkgmk -o -r "${PKG_DESTDIR}${PREFIX}" -d $PKGS
    ) > "${log}" 2>&1
    echo "$(time_stamp): done."

    run rm -rf "${PKG_DESTDIR}"
}

download_package()
{
    local url="$1"
    local base="$2"
    local tar_ball="${1##*/}"
    local log="${BUILDS}/${base}.log"

    mkdir -p "${BUILDS}"

    echo "$(time_stamp): Downloading ${base}..."
    if test ! -f "${SOURCES}/${tar_ball}"; then
	run wget -N -P "${SOURCES}" "$url" > "${log}"
    fi
    echo "$(time_stamp): done."
}

extract_package()
{
    local tar_ball="$1"
    local base="$2"
    local build_dir="${BUILDS}/${base}"
    local log="${BUILDS}/${base}.log"

    echo "$(time_stamp): Extracting ${base}..."
    run rm -rf "${build_dir}"
    run gtar xf "${SOURCES}/${tar_ball}" -C "${BUILDS}" > "${log}"
    echo "$(time_stamp): done."

    if test -d "${PATCHES}/${base}"; then
	for patch_file in $(echo "${PATCHES}/${base}/*" | sort); do
	    echo "$(time_stamp): Patching ${base} (${patch_file})..."
	    (
		cd "${build_dir}"
		run gpatch -p1 < ${patch_file}
	    ) > "${log}"
	    echo "$(time_stamp): done."
	done
    fi
}

install_pkg()
{
    local pkg_name="${PKG_PREFIX}${1}"
    local log="${PKGS}/${pkg_name}-install.log"

    echo "$(time_stamp): Installing package ${pkg_name}..."
    run_sudo pkgadd -d "${PKGS}/${PKG_NAME}.pkg" > "${log}" 2>&1
    echo "$(time_stamp): done."
}

build_package()
{
    local url="$1"
    local tar_ball="${1##*/}"
    local base="${tar_ball%.tar.*}"
    local build_dir="${BUILDS}/${base}"
    local dest_dir="${DEST}/${base}"
    local log="${BUILDS}/${base}.log"
    shift

    mkdir -p "${BUILDS}"

    download_package "$url" "$base"
    extract_package "$tar_ball" "$base"

    echo "$(time_stamp): Configuring ${base}..."
    (
        cd "${BUILDS}/${base}"
	if test -f "CMakeLists.txt"; then
	    run cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" "$@" .
	elif test -f "Configure"; then
	    run ./Configure --prefix="${PREFIX}" "$@"
	else
            run ./configure --enable-shared --prefix="${PREFIX}" "$@"
	fi
    ) > "${log}"
    echo "$(time_stamp): done."

    echo "$(time_stamp): Building ${base}..."
    run ${MAKE} -C "${build_dir}" > "${log}"
    echo "$(time_stamp): done."
}

install_package()
{
    local url="$1"
    local tar_ball="${1##*/}"
    local base="${tar_ball%.tar.*}"
    local build_dir="${BUILDS}/${base}"

    build_package "$@"
    build_pkg "${base}" "${build_dir}"
    install_pkg "${base}"
}
