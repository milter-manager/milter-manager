# -*- sh -*-
run()
{
    $@
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

install_package()
{
    url="$1"
    tarball="${1##*/}"
    base="${tarball%.tar.*}"
    time_stamp="${BUILDS}/${base}.time_stamp"
    shift

    mkdir -p "${BUILDS}"

    echo -n "Downloading ${base}..."
    run wget -N -P "${SOURCES}" "$url"
    echo done.

    echo -n "Extracting ${base}..."
    run gtar xf "${SOURCES}/${tarball}" -C "${BUILDS}"
    echo done.

    echo -n "Configuring ${base}..."
    (
        cd "${BUILDS}/${base}"
        run ./configure --enable-shared --prefix="${PREFIX}" "$@"
    )
    echo done.

    echo -n "Building ${base}..."
    run touch "${time_stamp}"
    run ${MAKE} -C "${BUILDS}/${base}"
    echo done.

    echo -n "Installing ${base}..."
    run ${MAKE} -C "${BUILDS}/${base}" install
    find $PREFIX -newer "${time_stamp}" -print | \
	pkgproto | \
	sed -e "s%$PREFIX/%%" -e "s/$USER other/root root/" > \
	"${BUILDS}/${base}.prototype"
    echo done.
}

build_pkg()
{
    local pkg_dir="${base_dir}/${1}"
    local pkg_name="${PKG_PREFIX}${1}"
    local archive_name="${pkg_name}.pkg"

    (
        cd $pkg_dir
        pkgmk -o -r $PREFIX -a `uname -p` -d $PKG_BASE_DIR
    )
}

