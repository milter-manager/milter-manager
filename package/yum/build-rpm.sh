#!/bin/sh

LANG=C

PACKAGE=$(cat /tmp/build-package)
VERSION=$(cat /tmp/build-version)
SOURCE_BASE_NAME=$(cat /tmp/build-source-base-name)
USER_NAME=$(cat /tmp/build-user)
DEPENDED_PACKAGES=$(cat /tmp/depended-packages)
USE_RPMFORGE=$(cat /tmp/build-use-rpmforge)
USE_ATRPMS=$(cat /tmp/build-use-atrpms)
USE_EPEL=$(cat /tmp/build-use-epel)
BUILD_OPTIONS=$(cat /tmp/build-options)
BUILD_SCRIPT=/tmp/build-${PACKAGE}.sh
BUILD_RUBY_SCRIPT=/tmp/build-ruby.sh

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

yum_options=

distribution=$(cut -d ' ' -f 1 /etc/redhat-release | tr 'A-Z' 'a-z')
if grep -q Linux /etc/redhat-release; then
    distribution_version=$(cut -d ' ' -f 4 /etc/redhat-release)
else
    distribution_version=$(cut -d ' ' -f 3 /etc/redhat-release)
fi
if ! rpm -q ${distribution}-release > /dev/null 2>&1; then
    packages_dir=/var/cache/yum/core/packages
    release_rpm=${distribution}-release-${distribution_version}-*.rpm
    run rpm -Uvh --force ${packages_dir}/${release_rpm}
    run rpm -Uvh --force ${packages_dir}/ca-certificates-*.rpm
fi

if test "$USE_RPMFORGE" = "yes"; then
    if ! rpm -q rpmforge-release > /dev/null 2>&1; then
	architecture=$(cut -d '-' -f 1 /etc/rpm/platform)
	rpmforge_url=http://packages.sw.be/rpmforge-release
	rpmforge_rpm_base=
	case $distribution_version in
		5.*)
			rpmforge_rpm_base=rpmforge-release-0.5.3-1.el5.rf.${architecture}.rpm
			;;
		6.*)
			rpmforge_rpm_base=rpmforge-release-0.5.3-1.el6.rf.${architecture}.rpm
			;;
	esac
	if test -n "$rpmforge_rpm_base"; then
	    wget $rpmforge_url/$rpmforge_rpm_base
	    run rpm -Uvh $rpmforge_rpm_base
	    rm $rpmforge_rpm_base
	    sed -i'' -e 's/enabled = 1/enabled = 0/g' /etc/yum.repos.d/rpmforge.repo
	fi
    fi
    yum_options="$yum_options --enablerepo=rpmforge"
fi

if test "$USE_ATRPMS" = "yes"; then
    case "$(cat /etc/redhat-release)" in
	CentOS*)
	    repository_label=CentOS
	    repository_prefix=el
	    ;;
	*)
	    repository_label=Fedora
	    repository_prefix=f
	    ;;
    esac
    cat <<EOF > /etc/yum.repos.d/atrpms.repo
[atrpms]
name=${repository_label} \$releasever - \$basearch - ATrpms
baseurl=http://dl.atrpms.net/${repository_prefix}\$releasever-\$basearch/atrpms/stable
gpgkey=http://ATrpms.net/RPM-GPG-KEY.atrpms
gpgcheck=1
enabled=0
EOF
    yum_options="$yum_options --enablerepo=atrpms"
fi

if test "$USE_EPEL" = "yes"; then
    if ! rpm -q epel-release > /dev/null 2>&1; then
	epel_url=
	case $distribution_version in
	    5.*)
		epel_url=http://ftp.iij.ad.jp/pub/linux/fedora/epel/5/i386/epel-release-5-4.noarch.rpm
		;;
	    6.*)
		epel_url=http://ftp.iij.ad.jp/pub/linux/fedora/epel/6/i386/epel-release-6-8.noarch.rpm
		;;
	esac
	if test -n "$epel_url"; then
	    run wget $epel_url
	    run rpm -Uvh $(basename $epel_url)
	    run rm $(basename $epel_url)
	    sed -i'' -e 's/enabled = 1/enabled = 0/g' /etc/yum.repos.d/epel.repo
	fi
    fi
    yum_options="$yum_options --enablerepo=epel"
fi

run yum update ${yum_options} -y
run yum install ${yum_options} -y rpm-build tar ${DEPENDED_PACKAGES}
run yum clean ${yum_options} packages

if ! id $USER_NAME >/dev/null 2>&1; then
    run useradd -m $USER_NAME
fi

case $distribution_version in
    6.*)
	if ! rpm -q ruby1.9 > /dev/null 2>&1; then
	    yum install -y wget libyaml libyaml-devel
            yum install -y $(grep BuildRequires: /tmp/ruby193.spec | cut -d: -f2)
            cat <<EOF > $BUILD_RUBY_SCRIPT
if [ ! -f ~/.rpmmacros ]; then
    cat <<EOM > ~/.rpmmacros
%_topdir \$HOME/rpm
EOM
fi

rm -rf rpm
mkdir -p rpm/SOURCES
mkdir -p rpm/SPECS
mkdir -p rpm/BUILD
mkdir -p rpm/RPMS
mkdir -p rpm/SRPMS

case $distribution_version in
  6.*)
    if ! rpm -q ruby1.9 > /dev/null 2>&1; then
      wget ftp://ftp.ruby-lang.org/pub/ruby/1.9/ruby-1.9.3-p392.tar.gz -P rpm/SOURCES
      cp -a /tmp/ruby193.spec rpm/SPECS/ruby193.spec
      rpmbuild -ba rpm/SPECS/ruby193.spec
    fi
    ;;
esac
EOF

            run chmod +x $BUILD_RUBY_SCRIPT
            run su - $USER_NAME $BUILD_RUBY_SCRIPT
            run rpm -Uvh ~$USER_NAME/rpm/RPMS/*/*.rpm
	fi
	;;
esac

cat <<EOF > $BUILD_SCRIPT
#!/bin/sh

if [ ! -f ~/.rpmmacros ]; then
    cat <<EOM > ~/.rpmmacros
%_topdir \$HOME/rpm
EOM
fi

mkdir -p rpm/SOURCES
mkdir -p rpm/SPECS
mkdir -p rpm/BUILD
mkdir -p rpm/RPMS
mkdir -p rpm/SRPMS

if test -f /tmp/${SOURCE_BASE_NAME}-$VERSION-*.src.rpm; then
    if ! rpm -Uvh /tmp/${SOURCE_BASE_NAME}-$VERSION-*.src.rpm; then
        cd rpm/SOURCES
        rpm2cpio /tmp/${SOURCE_BASE_NAME}-$VERSION-*.src.rpm | cpio -id
        if ! yum info tcp_wrappers-devel >/dev/null 2>&1; then
            sed -i'' -e 's/tcp_wrappers-devel/tcp_wrappers/g' ${PACKAGE}.spec
        fi
        if ! yum info libdb-devel >/dev/null 2>&1; then
            sed -i'' -e 's/libdb-devel/db4-devel/g' ${PACKAGE}.spec
        fi
        sed -i'' -e 's/BuildArch: noarch//g' ${PACKAGE}.spec
        mv ${PACKAGE}.spec ../SPECS/
        cd
    fi
else
    cp /tmp/${SOURCE_BASE_NAME}-$VERSION.* rpm/SOURCES/
    cp /tmp/${PACKAGE}.spec rpm/SPECS/
fi

chmod o+rx . rpm rpm/RPMS rpm/SRPMS

rpmbuild -ba rpm/SPECS/${PACKAGE}.spec ${BUILD_OPTIONS}
EOF

run chmod +x $BUILD_SCRIPT
run su - $USER_NAME $BUILD_SCRIPT
