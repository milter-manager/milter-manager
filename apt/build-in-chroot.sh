#!/bin/sh

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

    run_sudo debootstrap --arch $architecture $code_name $base_dir

    case $code_name in
	lenny|squeeze|unstable)
	    run_sudo sed -i'' -e 's/us/jp/' $base_dir/etc/apt/sources.list
	    ;;
	*)
	    run_sudo sed -i'' -e 's/main$/main universe/' \
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
	lenny|squeeze|unstable)
	    distribution=debian
	    component=main
	    ;;
	*)
	    distribution=ubuntu
	    component=universe
	    ;;
    esac

    source_dir=${script_base_dir}/..
    build_user=${PACKAGE}-build
    build_user_dir=${base_dir}/home/$build_user
    build_dir=${build_user_dir}/build
    if ruby -e "exit(('$VERSION'.split(/\./)[1].to_i % 2).zero?)"; then
	status=stable
    else
	status=development
    fi
    package_initial=$(echo ${PACKAGE} | sed -e 's/\(.\).*/\1/')
    pool_dir=${script_base_dir}/${distribution}/${status}/pool
    pool_dir=${pool_dir}/${code_name}/${component}/${package_initial}/${PACKAGE}
    run cp $source_dir/${PACKAGE}-${VERSION}.tar.gz \
	${CHROOT_BASE}/$target/tmp/
    run rm -rf ${CHROOT_BASE}/$target/tmp/${PACKAGE}-debian
    run cp -rp $source_dir/debian/ \
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
    run_sudo su -c "/usr/sbin/chroot ${CHROOT_BASE}/$target /tmp/build-deb.sh"
    run mkdir -p $pool_dir
    run echo 'Options +Indexes' > ${script_base_dir}/${distribution}/.htaccess
    for path in $build_dir/*; do
	[ -f $path ] && run cp -p $path $pool_dir/
    done
}

for architecture in $ARCHITECTURES; do
    for code_name in $CODES; do
	build $architecture $code_name
    done;
done
