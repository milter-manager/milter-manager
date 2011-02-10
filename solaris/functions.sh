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
    run ${MAKE} -C "${build_dir}/${base}"
    echo done.

    echo -n "Installing ${base}..."
    run touch /tmp/timestamp
    run ${MAKE} -C "${build_dir}/${base}" install
    find $PREFIX -newer /tmp/timestamp -print | pkgproto | sed -e "s\!$PREFIX/\!\!" > "/tmp/prototype-${base}"
    echo done.
}

