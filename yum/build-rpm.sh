#!/bin/sh

LANG=C

PACKAGE=$(cat /tmp/build-package)
VERSION=$(cat /tmp/build-version)
SOURCE_BASE_NAME=$(cat /tmp/build-source-base-name)
USER_NAME=$(cat /tmp/build-user)
DEPENDED_PACKAGES=$(cat /tmp/depended-packages)
USE_RPMFORGE=$(cat /tmp/build-use-rpmforge)
BUILD_SCRIPT=/tmp/build-${PACKAGE}.sh

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
distribution_version=$(cut -d ' ' -f 3 /etc/redhat-release)
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
	rpmforge_rpm_base=rpmforge-release-0.5.2-2.el5.rf.${architecture}.rpm
	run rpm -Uvh $rpmforge_url/$rpmforge_rpm_base
	sed -i'' -e 's/enabled = 1/enabled = 0/g' /etc/yum.repos.d/rpmforge.repo
    fi
    yum_options="$yum_options --enablerepo=rpmforge"
fi

run yum update ${yum_options} -y
run yum install ${yum_options} -y rpm-build tar ${DEPENDED_PACKAGES}
run yum clean ${yum_options} packages

if ! id $USER_NAME >/dev/null 2>&1; then
    run useradd -m $USER_NAME
fi

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

cp /tmp/${SOURCE_BASE_NAME}-$VERSION.* rpm/SOURCES/
cp /tmp/${PACKAGE}.spec rpm/SPECS/

chmod o+rx . rpm rpm/RPMS rpm/SRPMS

rpmbuild -ba rpm/SPECS/${PACKAGE}.spec
EOF

run chmod +x $BUILD_SCRIPT
run su - $USER_NAME $BUILD_SCRIPT
