#!/bin/sh
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

build_chroot()
{
    architecture=$1
    code_name=$2

    case $code_name in
        lucid)
            case $architecture in
                i386)
                    machine=i386
                    ;;
                amd64)
                    machine=x86_64
                    ;;
            esac
            run_sudo setarch $machine --uname-2.6 debootstrap --arch $architecture $code_name $base_dir http://jp.archive.ubuntu.com/ubuntu
            ;;
        *)
            run_sudo debootstrap --arch $architecture $code_name $base_dir
            ;;
    esac

    case $code_name in
        wheezy|jessie|unstable)
            run_sudo sed -i'' -e 's/us/jp/' $base_dir/etc/apt/sources.list
            case $code_name in
                wheezy)
                    run_sudo sed -i'' \
                        -e "\$a\\deb http://security.debian.org/ ${code_name}/updates main" \
                        $base_dir/etc/apt/sources.list
                    ;;
                *)
                    # do nothing
                    ;;
            esac
            ;;
        *)
            run_sudo sed -i'' \
                -e 's/main$/main universe/' \
                -e 's,http://archive,http://jp.archive,' \
                -e "\$a\\deb http://security.ubuntu.com/ubuntu ${code_name}-security main" \
                $base_dir/etc/apt/sources.list
            ;;
    esac

    run_sudo sh -c "echo >> /etc/fstab"
    run_sudo sh -c "echo /sys ${base_dir}/sys none bind 0 0 >> /etc/fstab"
    run_sudo sh -c "echo /dev ${base_dir}/dev none bind 0 0 >> /etc/fstab"
    run_sudo sh -c "echo devpts-chroot ${base_dir}/dev/pts devpts defaults 0 0 >> /etc/fstab"
    run_sudo sh -c "echo proc-chroot ${base_dir}/proc proc defaults 0 0 >> /etc/fstab"
    run_sudo mount ${base_dir}/sys
    run_sudo mount ${base_dir}/dev
    run_sudo mount ${base_dir}/dev/pts
    run_sudo mount ${base_dir}/proc
}

build()
{
    architecture=$1
    code_name=$2

    target=${code_name}-${architecture}
    base_dir=${CHROOT_BASE}/${target}
    if [ ! -d $base_dir ]; then
        run build_chroot $architecture $code_name
    fi

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

    source_dir=${script_base_dir}/../..
    build_user=${PACKAGE}-build
    build_user_dir=${base_dir}/home/$build_user
    build_dir=${build_user_dir}/build
    status=stable
    package_initial=$(echo ${PACKAGE} | sed -e 's/\(.\).*/\1/')
    pool_dir=${script_base_dir}/${distribution}/${status}/pool
    pool_dir=${pool_dir}/${code_name}/${component}/${package_initial}/${PACKAGE}
    run cp $source_dir/${PACKAGE}-${VERSION}.tar.gz \
        ${CHROOT_BASE}/$target/tmp/
    run rm -rf ${CHROOT_BASE}/$target/tmp/${PACKAGE}-debian
    run cp -rp ${source_dir}/package/debian/ \
        ${CHROOT_BASE}/$target/tmp/${PACKAGE}-debian
    run find ${CHROOT_BASE}/$target/tmp/${PACKAGE}-debian/ \
        -name ".svn" -print0 | xargs -0 -r rm -rf \{\} \;
    run echo $PACKAGE > ${CHROOT_BASE}/$target/tmp/build-package
    run echo $VERSION > ${CHROOT_BASE}/$target/tmp/build-version
    run echo $build_user > ${CHROOT_BASE}/$target/tmp/build-user
    run cp ${script_base_dir}/${PACKAGE}-depended-packages \
        ${CHROOT_BASE}/$target/tmp/depended-packages
    run cp ${script_base_dir}/build-deb.sh \
        ${CHROOT_BASE}/$target/tmp/
    run_sudo rm -rf $build_dir
    case $code_name in
        lucid)
            case $architecture in
                i386)
                    machine=i386
                    ;;
                amd64)
                    machine=x86_64
                    ;;
            esac
            run_sudo su -c "setarch $machine --uname-2.6 /usr/sbin/chroot ${CHROOT_BASE}/$target /tmp/build-deb.sh"
            ;;
        *)
            run_sudo su -c "/usr/sbin/chroot ${CHROOT_BASE}/$target /tmp/build-deb.sh"
            ;;
    esac
    run mkdir -p $pool_dir
    run echo 'Options +Indexes' > ${script_base_dir}/${distribution}/.htaccess
    for path in $build_dir/*; do
        [ -f $path ] && run cp -p $path $pool_dir/
    done
}

for architecture in $ARCHITECTURES; do
    for code_name in $CODES; do
        if test "$parallel" = "yes"; then
            build $architecture $code_name &
        else
            build $architecture $code_name
        fi;
    done;
done

if test "$parallel" = "yes"; then
    wait
fi
