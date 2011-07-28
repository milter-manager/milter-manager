# -*- sh -*-

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

run_pfexec()
{
    run pfexec "$@"
}

time_stamp()
{
    date +%Y-%m-%dT%H:%m:%S
}

check_pkgs()
{
    echo "$(time_stamp): Checking libraries..."
    check_pkg "${PKG_PREFIX}ruby"
    check_pkg "${PKG_PREFIX}iconv"
    check_pkg "${PKG_PREFIX}gettext"
    check_pkg "${PKG_PREFIX}glib"
    check_pkg "${PKG_PREFIX}mysql"
    check_pkg "${PKG_PREFIX}cutter"
    echo "$(time_stamp): done."
}

check_pkg()
{
    pkg="$1"
    if pkginfo | ggrep -q "$pkg"; then
        echo "already installed ${pkg}."
    else
        echo "not installed ${pkg} yet."
        exit 1
    fi
    if test -d "${PKGS}/${pkg}"; then
        echo "already exist ${pkg}."
    else
        echo "does not exist ${pkg}."
        exit 1
    fi
}

build_pkg()
{
    local base="$1"
    local build_dir="$2"
    local package_name="${base%%-[0-9]*}"
    package_name="${package_name##lib}"
    local prototype_dir="${PROTOTYPES}/${package_name}"
    local prototype="${prototype_dir}/prototype"
    local preinstall="${prototype_dir}/preinstall"
    local postinstall="${prototype_dir}/postinstall"
    local user="$(/usr/xpg4/bin/id -un)"
    local group="$(/usr/xpg4/bin/id -gn)"
    local log="${BUILDS}/${base}.log"
    shift
    shift

    create_pkginfo "${base}" "${build_dir}"
    update_prototype "${base}" "${build_dir}" "$@"

    echo "$(time_stamp): Building pkg ${package_name}..."
    run mkdir -p "${PKGS}"
    (
        cd "$prototype_dir"
        run pkgmk -o -r "${PKG_DESTDIR}${PREFIX}" -d "$PKGS"
    ) > "${log}" 2>&1
    echo "$(time_stamp): done."

    run rm -rf "${PKG_DESTDIR}"
}

create_pkginfo()
{
    local base="$1"
    local build_dir="$2"
    local package_name="${base%%-[0-9]*}"
    package_name="${package_name##lib}"
    local prototype_dir="${PROTOTYPES}/${package_name}"
    local pkginfo_template="${prototype_dir}/pkginfo.in"
    local pkginfo="${prototype_dir}/pkginfo"
    local build_dir="${base_dir}/../"
    local version=$(PKG_CONFIG_PATH="$build_dir" pkg-config --modversion milter-manager)
    shift
    shift

    echo "$(time_stamp): Creating pkginfo of ${base}..."
    sed -e "s,@prefix@,${PREFIX},g" \
        -e "s,@VERSION@,${version},g" \
        "${pkginfo_template}" > "${pkginfo}"
    echo "$(time_stamp): done."
}

update_prototype()
{
    local base="$1"
    local build_dir="$2"
    local package_name="${base%%-[0-9]*}"
    package_name="${package_name##lib}"
    local prototype_dir="${PROTOTYPES}/${package_name}"
    local prototype="${prototype_dir}/prototype"
    local preinstall="${prototype_dir}/preinstall"
    local postinstall="${prototype_dir}/postinstall"
    local user="$(/usr/xpg4/bin/id -un)"
    local group="$(/usr/xpg4/bin/id -gn)"
    local log="${BUILDS}/${base}.log"
    shift
    shift

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
}

install_pkg()
{
    local base="$1"
    local package_name="${base%%-[0-9]*}"
    package_name="${package_name##lib}"
    local pkg_name="${PKG_PREFIX}${package_name}"
    local log="${PKGS}/${pkg_name}-install.log"

    if test $(zonename) = global; then
        echo "$(time_stamp): Installing pkg ${pkg_name}..."
        yes | run_pfexec /usr/sbin/pkgadd -G -d "${PKGS}" "${pkg_name}" > "${log}" 2>&1
        echo "$(time_stamp): done."
    else
        echo "$(time_stamp): Installing pkg ${pkg_name}..."
        yes | run_pfexec /usr/sbin/pkgadd -d "${PKGS}" "${pkg_name}" > "${log}" 2>&1
        echo "$(time_stamp): done."
    fi
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
