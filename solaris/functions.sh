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
    build_dir="${PREFIX}/build"
    shift

    mkdir -p "$build_dir"

    echo -n "Downloading ${base}..."
    run wget -N -P "${SOURCES}" "$url"
    echo done.

    echo -n "Extracting ${base}..."
    run gtar xf "${SOURCES}/${tarball}" -C "${build_dir}"
    echo done.

    echo -n "Configuring ${base}..."
    (
        cd "${build_dir}/${base}"
        run ./configure --enable-shared --prefix="${PREFIX}" "$@"
    )
    echo done.

    echo -n "Building ${base}..."
    run touch /tmp/timestamp
    run ${MAKE} -C "${build_dir}/${base}"
    echo done.

    echo -n "Installing ${base}..."
    run ${MAKE} -C "${build_dir}/${base}" install
    find $PREFIX -newer /tmp/timestamp -print | pkgproto | sed -e "s%$PREFIX/%%" -e "s/$USER other/root root/"> "/tmp/prototype-${base}"
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

