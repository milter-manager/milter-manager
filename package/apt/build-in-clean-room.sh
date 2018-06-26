#!/bin/bash
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

if [ $# != 4 ]; then
    echo "Usage: $0 VERSION CLEAN_ROOM_BASE ARCHITECTURES CODES"
    echo " e.g.: $0 2.0.5 /var/cache/pbuilder 'i386 amd64' 'wheezy jessie unstable'"
    exit 1
fi

PACKAGE=milter-manager
VERSION=$1
CHROOT_BASE=$2
ARCHITECTURES=$3
CODES=$4

PATH=/usr/local/sbin:/usr/sbin:$PATH

script_base_dir=`dirname $0`

if test "$PARALLEL" = "yes"; then
    parallel="yes"
else
    parallel="no"
fi

run()
{
    "$@"
    if test $? -ne 0; then
        echo "Failed $@"
        exit 1
    fi
}

run_sudo()
{
    run sudo "$@"
}

build_by_pbuilder()
{
    architecture=$1
    code_name=$2
    source_dir=${script_base_dir}/../..
    case ${code_name} in
        stretch|buster|unstable)
            distribution=debian
            component=main
            ;;
        *)
            distribution=ubuntu
            component=universe
            ;;
    esac
    status=stable
    package_initial=$(echo ${PACKAGE} | sed -e 's/\(.\).*/\1/')
    pool_dir=${script_base_dir}/${distribution}/${status}/pool
    pool_dir=${pool_dir}/${code_name}/${component}/${package_initial}/${PACKAGE}
    basetgz=$CHROOT_BASE/$code_name-$architecture-base.tgz
    builddir=${script_base_dir}/$code_name-$architecture
    aptcache_dir=$CHROOT_BASE/aptcache/$code_name-$architecture

    run_sudo mkdir -p $aptcache_dir

    OPTS=( )
    OPTS+=( --distribution "$code_name" )
    OPTS+=( --architecture "$architecture" )
    OPTS+=( --basetgz "$basetgz" )
    OPTS+=( --aptcache "$aptcache_dir" )
    case $code_name in
        trusty|xenial|zesty|artful|bionic)
            OPTS+=( --components 'main universe' )
            MIRROR=http://jp.archive.ubuntu.com/ubuntu
            OPTS+=( --mirror "$MIRROR" )
            OPTS+=( --othermirror "deb $MIRROR $code_name-security main universe" )
            OPTS+=( --debootstrapopts --keyring=/usr/share/keyrings/ubuntu-archive-keyring.gpg )
            ;;
        buster|stretch)
            MIRROR=http://ftp.jp.debian.org/debian
            OPTS+=( --mirror "$MIRROR" )
            OPTS+=( --othermirror "deb http://security.debian.org $code_name/updates main" )
            OPTS+=( --components main )
            ;;
        unstable|*)
            MIRROR=http://ftp.jp.debian.org/debian
            OPTS+=( --mirror "$MIRROR" )
    esac
    if [ ! -s $basetgz ] ; then
        run_sudo pbuilder --create "${OPTS[@]}"
    fi
    run_sudo pbuilder --update "${OPTS[@]}"

    mkdir -p $builddir
    run mkdir -p $pool_dir
    run echo 'Options +Indexes' > ${script_base_dir}/${distribution}/.htaccess
    run tar xf $source_dir/${PACKAGE}-${VERSION}.tar.gz -C $builddir
    ( cd $builddir && \
        run ln -sf ../../../${PACKAGE}-${VERSION}.tar.gz  \
        ${PACKAGE}_${VERSION}.orig.tar.gz
    )
    run cp -rp ${source_dir}/package/debian $builddir/${PACKAGE}-${VERSION}/
    if test "$SKIP_TEST" = "yes"; then
        ( cd $builddir/${PACKAGE}-${VERSION}/ && \
            sed -i -s -e '/cutter/d' debian/control && \
            sed -i -s -e '/TZ=Asia/d' debian/rules
        )
    fi

    ( cd $builddir/${PACKAGE}-${VERSION} && \
        run_sudo pdebuild \
            --pbuilder pbuilder --buildresult ../../${pool_dir}/  \
            -- "${OPTS[@]}"
    )
    run_sudo chown `id -u`:`id -g` -R $pool_dir/
    run_sudo rm -fr $builddir
}

: ${TMPFS_SIZE:=4g}
buildplace=$CHROOT_BASE/build/
if test "$USE_TMPFS" = "yes"; then
    run_sudo mount -t tmpfs -o size=$TMPFS_SIZE tmpfs $buildplace
fi

for architecture in $ARCHITECTURES; do
    for code_name in $CODES; do
        if test "$parallel" = "yes"; then
            build_by_pbuilder $architecture $code_name &
        else
            build_by_pbuilder $architecture $code_name
        fi
    done
done

if test "$parallel" = "yes" ; then
    wait
fi

if test "$USE_TMPFS" = "yes"; then
    run_sudo umount $buildplace
fi
