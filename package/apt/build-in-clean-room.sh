#!/bin/bash
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

if [ $# != 4 ]; then
    echo "Usage: $0 VERSION CHROOT_BASE ARCHITECTURES CODES"
    echo " e.g.: $0 1.5.0 /var/lib/chroot 'i386 amd64' 'lenny hardy kermic'"
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
        wheezy|jessie|unstable)
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
    basetgz=$CHROOT_BASE/base-$code_name-$architecture.tgz
    builddir=${script_base_dir}/$code_name-$architecture

    OPTS=( )
    OPTS+=( --distribution "$code_name" )
    OPTS+=( --architecture "$architecture" )
    case $code_name in
        lucid)
            OPTS+=( --components 'main universe' )
            OPTS+=( --basetgz "$basetgz" )
            MIRROR=http://old-release.archive.ubuntu.com/ubuntu
            OPTS+=( --mirror "$MIRROR" )
            OPTS+=( --othermirror "deb $MIRROR $code_name-security main universe")
            OPTS+=( --debootstrapopts --keyring=/usr/share/keyrings/ubuntu-archive-keyring.gpg )
            ;;
        precise|trusty)
            OPTS+=( --components 'main universe' )
            OPTS+=( --basetgz "$basetgz" )
            MIRROR=http://jp.archive.ubuntu.com/ubuntu
            OPTS+=( --mirror "$MIRROR" )
            OPTS+=( --othermirror "deb $MIRROR $code_name-security main universe")
            OPTS+=( --debootstrapopts --keyring=/usr/share/keyrings/ubuntu-archive-keyring.gpg )
            ;;
        wheezy|jessie)
            OPTS+=( --basetgz "$basetgz" )
            MIRROR=http://ftp.jp.debian.org/debian
            OPTS+=( --mirror "$MIRROR" )
            OPTS+=( --othermirror "deb http://security.debian.org $code_name/updates main")
            ;;
        unstable|*)
            OPTS+=( --basetgz "$basetgz" )
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

    case $code_name in
        lucid)
            sed -i -e 's/ruby (>= 1:1.9.3)/ruby1.9.1 (>= 1.9.1)/g' $builddir/${PACKAGE}-${VERSION}/debian/control
            sed -i -e 's/ruby-dev (>= 1:1.9.3)/ruby1.9.1-dev (>= 1.9.1)/g' $builddir/${PACKAGE}-${VERSION}/debian/control
            sed -i -e 's/debhelper (>= 9)/debhelper (>= 7)/' $builddir/${PACKAGE}-${VERSION}/debian/control
            sed -i -e 's/9/7/' $builddir/${PACKAGE}-${VERSION}/debian/compat
            sed -i -e 's,usr/lib/\*,usr/lib,' $builddir/${PACKAGE}-${VERSION}/debian/*.install
            ;;
        precise)
            sed -i -e 's/ruby (>= 1:1.9.3)/ruby1.9.1 (>= 1.9.1)/g' $builddir/${PACKAGE}-${VERSION}/debian/control
            sed -i -e 's/ruby-dev (>= 1:1.9.3)/ruby1.9.1-dev (>= 1.9.1)/g' $builddir/${PACKAGE}-${VERSION}/debian/control
            ;;
        *)
            ;;
    esac
    ( cd $builddir/${PACKAGE}-${VERSION} && \
        run_sudo pdebuild \
            --pbuilder pbuilder --buildresult ../../${pool_dir}/  \
            -- "${OPTS[@]}"
    )
    run_sudo chown `id -u`:`id -g` -R $pool_dir/
    run_sudo rm -fr $builddir
}

for architecture in $ARCHITECTURES; do
    for code_name in $CODES; do
        if test "$parallel" = "yes"; then
            build_by_pbuilder $architecture $code_name &
        else
            build_by_pbuilder $architecture $code_name
        fi;
    done;
done

if test "$parallel" = "yes" ; then
    wait
fi
