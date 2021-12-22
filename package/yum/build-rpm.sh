#!/bin/sh
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

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
            6.*)
                rpmforge_rpm_base=rpmforge-release-0.5.3-1.el6.rf.${architecture}.rpm
                ;;
            7.*)
                # repoforge.org supports only x86_64 for now.
                rpmforge_rpm_base=rpmforge-release-0.5.3-1.el7.rf.${architecture}.rpm
                ;;
        esac
        if test -n "$rpmforge_rpm_base"; then
            run yum update ${yum_options} -y
            run yum install ${yum_options} -y wget
            run yum ${yum_options} clean packages
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
        run yum update ${yum_options} -y
        run yum install ${yum_options} -y wget pyliblzma ca-certificates
        run yum ${yum_options} clean packages
        run yum install -y epel-release
        sed -i'' -e 's/enabled = 1/enabled = 0/g' /etc/yum.repos.d/epel.repo
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
        if ! rpm -q ruby2.3 > /dev/null 2>&1; then
            yum install -y wget
            yum install -y $(grep BuildRequires: /tmp/ruby23.spec | cut -d: -f2)
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
    if ! rpm -q ruby2.3 > /dev/null 2>&1; then
      wget http://cache.ruby-lang.org/pub/ruby/2.3/ruby-2.3.5.tar.gz -P rpm/SOURCES
      cp -a /tmp/ruby23.spec rpm/SPECS/ruby23.spec
      rpmbuild -ba rpm/SPECS/ruby23.spec
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
mkdir -p .config

echo "from Config import *" > .config/rpmlint
echo 'addFilter("E: non-readable /etc/cron.d/milter-manager-log")' >> .config/rpmlint

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

source /opt/rh/rh-ruby30/enable
rpmbuild -ba rpm/SPECS/${PACKAGE}.spec ${BUILD_OPTIONS}
find rpm/RPMS -name \*${VERSION}\*.rpm | xargs rpmlint
:
EOF

run chmod +x $BUILD_SCRIPT
run su - $USER_NAME $BUILD_SCRIPT
